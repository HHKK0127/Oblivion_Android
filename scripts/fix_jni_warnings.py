#!/usr/bin/env python3
"""
Fix JNI compiler warnings by properly placing [[maybe_unused]] attributes
"""

import re

def fix_jni_bridge():
    """Fix [[maybe_unused]] attribute placement in jni_bridge.cpp"""

    file_path = "app/src/main/cpp/jni_bridge.cpp"

    with open(file_path, 'r') as f:
        content = f.read()

    # Pattern: attribute after type (correct way)
    # Replace: JNIEnv* [[maybe_unused]] env -> [[maybe_unused]] JNIEnv* env

    patterns = [
        (r'JNIEnv\* \[\[maybe_unused\]\] env', '[[maybe_unused]] JNIEnv* env'),
        (r'jobject \[\[maybe_unused\]\] obj', '[[maybe_unused]] jobject obj'),
    ]

    for old, new in patterns:
        content = re.sub(old, new, content)

    with open(file_path, 'w') as f:
        f.write(content)

    print(f"✅ Fixed {file_path}")

if __name__ == "__main__":
    fix_jni_bridge()
