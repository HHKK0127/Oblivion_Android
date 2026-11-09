use serde::{Deserialize, Serialize};
use thiserror::Error;

#[derive(Debug, Error)]
pub enum EssError {
    #[error("not an ESS file: bad magic")]
    BadMagic,
    #[error("unexpected end of file")]
    Truncated,
}

/// Header block: 12-byte magic, version numbers, timestamps, player metadata.
#[derive(Debug, Serialize, Deserialize)]
pub struct EssHeader {
    pub version: u32,
    pub player_name: String,
}

pub fn parse_header(data: &[u8]) -> Result<EssHeader, EssError> {
    if data.len() < 16 || &data[0..12] != b"TES4SAVEGAME" {
        return Err(EssError::BadMagic);
    }
    let version = u32::from_le_bytes([data[12], data[13], data[14], data[15]]);
    // TODO: parse length-prefixed player name and full header fields
    Ok(EssHeader {
        version,
        player_name: String::new(),
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rejects_bad_magic() {
        assert!(matches!(parse_header(b"hello"), Err(EssError::BadMagic)));
    }
}
