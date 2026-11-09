//! Little-endian cursor over a resident NIF buffer.
//!
//! The original C++ reader keeps one sticky `readError` flag: once a read runs
//! past the end of the buffer every later read fails and hands back a zero
//! value. This module mirrors that behaviour so the walker can stay a direct
//! transcription of `nif_parser.cpp` instead of threading `Option` through
//! every helper.

/// Upper bound the original reader accepts for one SizedString, in bytes.
pub(crate) const MAX_STRING_BYTES: u32 = 1024 * 1024;

pub(crate) struct Reader<'a> {
    data: &'a [u8],
    pos: usize,
    error: bool,
}

impl<'a> Reader<'a> {
    pub(crate) fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            pos: 0,
            error: false,
        }
    }

    /// A reader that starts at `pos`, used by the phases that resume at the
    /// block data offset rather than at the start of the file.
    pub(crate) fn with_position(data: &'a [u8], pos: usize) -> Self {
        Self {
            data,
            pos: pos.min(data.len()),
            error: false,
        }
    }

    pub(crate) fn pos(&self) -> usize {
        self.pos
    }

    pub(crate) fn set_pos(&mut self, pos: usize) {
        self.pos = pos;
    }

    pub(crate) fn len(&self) -> usize {
        self.data.len()
    }

    pub(crate) fn data(&self) -> &'a [u8] {
        self.data
    }

    pub(crate) fn has_error(&self) -> bool {
        self.error
    }

    pub(crate) fn clear_error(&mut self) {
        self.error = false;
    }

    /// Raise the sticky flag without moving the cursor. Used where the C++ code
    /// assigns `readError = true` directly (a size guard that is not an I/O
    /// failure but still has to stop the walk).
    pub(crate) fn set_error(&mut self) {
        self.error = true;
    }

    /// Copy `count` bytes into `out`, or raise the sticky flag.
    pub(crate) fn read_bytes(&mut self, out: &mut [u8]) -> bool {
        if self.error {
            return false;
        }
        let end = match self.pos.checked_add(out.len()) {
            Some(end) => end,
            None => {
                self.error = true;
                return false;
            }
        };
        if end > self.data.len() {
            self.error = true;
            return false;
        }
        out.copy_from_slice(&self.data[self.pos..end]);
        self.pos = end;
        true
    }

    /// Advance the cursor without copying, or raise the sticky flag.
    pub(crate) fn skip(&mut self, count: usize) -> bool {
        if self.error {
            return false;
        }
        let end = match self.pos.checked_add(count) {
            Some(end) => end,
            None => {
                self.error = true;
                return false;
            }
        };
        if end > self.data.len() {
            self.error = true;
            return false;
        }
        self.pos = end;
        true
    }

    pub(crate) fn u8(&mut self) -> u8 {
        let mut bytes = [0u8; 1];
        if self.read_bytes(&mut bytes) {
            bytes[0]
        } else {
            0
        }
    }

    pub(crate) fn u16(&mut self) -> u16 {
        let mut bytes = [0u8; 2];
        if self.read_bytes(&mut bytes) {
            u16::from_le_bytes(bytes)
        } else {
            0
        }
    }

    pub(crate) fn u32(&mut self) -> u32 {
        let mut bytes = [0u8; 4];
        if self.read_bytes(&mut bytes) {
            u32::from_le_bytes(bytes)
        } else {
            0
        }
    }

    pub(crate) fn f32(&mut self) -> f32 {
        f32::from_bits(self.u32())
    }

    // -----------------------------------------------------------------
    // Array and record skips shared by the walker and the block-anchored
    // readers. They only advance the cursor; the sticky flag records a read
    // that ran past the end of the buffer.
    // -----------------------------------------------------------------

    pub(crate) fn skip_ref_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(4));
    }

    pub(crate) fn skip_vector3_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(12));
    }

    /// SizedString / ByteArray / FilePath all share the u32 length prefix
    /// layout. Unlike [`string`](Self::string) this applies no size guard: the
    /// length is used to skip, and a length past the end of the file fails the
    /// skip.
    pub(crate) fn skip_string_array(&mut self, count: u32) {
        for _ in 0..count {
            if self.error {
                break;
            }
            let length = self.u32();
            if self.error {
                break;
            }
            self.skip(length as usize);
        }
    }

    /// NiObjectNET: Name | Extra Data List | Controller.
    ///
    /// Pre-5.0.0.1 meshes store Extra Data as a single ref with no list count,
    /// which only arrived with the 10.0.1.0 layout.
    pub(crate) fn skip_ni_object_net(&mut self, has_block_type_table: bool) {
        self.skip_string_array(1);
        if !has_block_type_table {
            self.skip(4); // Extra Data
            self.skip(4); // Controller
            return;
        }
        let extra_data_count = self.u32();
        if self.error {
            return;
        }
        self.skip_ref_array(extra_data_count);
        self.skip(4);
    }

    /// SizedString: a u32 length followed by exactly that many bytes.
    ///
    /// A length above [`MAX_STRING_BYTES`] fails without raising the sticky
    /// flag, matching the original reader: the caller reports the bad string
    /// but the cursor is left where the length was read.
    pub(crate) fn string(&mut self) -> Result<String, ()> {
        let length = self.u32();
        if self.error {
            return Err(());
        }
        if length == 0 {
            return Ok(String::new());
        }
        if length > MAX_STRING_BYTES {
            return Err(());
        }
        let mut bytes = vec![0u8; length as usize];
        if !self.read_bytes(&mut bytes) {
            return Err(());
        }
        // NIF names are ASCII in practice. A name that is not valid UTF-8 only
        // loses fidelity in its text form; every offset is derived from the
        // stored length, so the walk is unaffected.
        Ok(String::from_utf8_lossy(&bytes).into_owned())
    }

    /// The original's `readStringRef`: a u32 string-table index with no table
    /// to look it up in — the placeholder `string_<index>` is the value it
    /// hands back, and the differential oracle compares that placeholder byte
    /// for byte, so the quirk is reproduced exactly. A read that runs past the
    /// end fails without touching the output, just as the original returns
    /// false and leaves its out-parameter alone.
    pub(crate) fn string_ref(&mut self) -> Result<String, ()> {
        let index = self.u32();
        if self.error {
            return Err(());
        }
        Ok(format!("string_{index}"))
    }

    /// BzString: a u8 length that counts the terminating NUL, then the body.
    /// Trailing NULs are stripped, so an empty body reads back as an empty
    /// string.
    pub(crate) fn byte_string(&mut self) -> Result<String, ()> {
        let length = self.u8();
        if self.error {
            return Err(());
        }
        if length == 0 {
            return Ok(String::new());
        }
        let mut bytes = vec![0u8; length as usize];
        if !self.read_bytes(&mut bytes) {
            return Err(());
        }
        while bytes.last() == Some(&0) {
            bytes.pop();
        }
        Ok(String::from_utf8_lossy(&bytes).into_owned())
    }
}
