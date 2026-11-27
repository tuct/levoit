"""Build the protocol core with sanitizers and run its host test.

The parser, ACK and version-guard cases this file used to carry (compiled by
slicing levoit.cpp as text) now live in tests/host/levoit_device_test.cpp,
which drives the real LevoitDevice and decoders through a fake platform and
host. This wrapper builds that test with AddressSanitizer and
UndefinedBehaviorSanitizer, which the plain g++ build in host-tests.yml does
not, so a memory error in the core fails here.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / 'components' / 'levoit'
CORE_SOURCES = [
    'levoit_device.cpp', 'decoder.cpp',
    'core_status.cpp', 'vital_status.cpp', 'sprout_status.cpp', 'superior_status.cpp',
    'core_commands.cpp', 'vital_commands.cpp', 'sprout_commands.cpp', 'superior_commands.cpp',
    'tlv.cpp', 'levoit_message.cpp',
]


class UARTIntegrityTests(unittest.TestCase):
    def test_core_under_sanitizers(self):
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp) / 'levoit-device-test'
            subprocess.run([shutil.which('clang++') or 'c++', '-std=c++17', '-Wall', '-Wextra',
                            '-fno-sanitize-recover=all',
                            '-fsanitize=' + os.environ.get('LEVOIT_TEST_SANITIZERS', 'address,undefined'),
                            str(ROOT / 'tests' / 'host' / 'levoit_device_test.cpp'),
                            *[str(COMPONENT / s) for s in CORE_SOURCES],
                            '-o', str(exe)], check=True)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            # Captured tag 14 = 1F 1C 01 01 01: byte 2 is a flag, not time.
            self.assertIn('DryRemaining=7199 sec, AutoDry=1, DryActive=1, DryLevel=1',
                          result.stderr)


if __name__ == '__main__': unittest.main()
