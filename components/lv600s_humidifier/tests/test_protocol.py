"""Native tests use synthetic frames, no credentials or device access."""

from pathlib import Path
import os
import subprocess
import tempfile
import unittest


class ProtocolTest(unittest.TestCase):
    def test_protocol(self):
        with tempfile.TemporaryDirectory() as temp:
            exe = str(Path(temp) / "lv600s-test")
            source = Path(__file__).with_name("protocol_test.cpp")
            subprocess.run(
                [
                    "c++",
                    "-std=c++17",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-fsanitize="
                    + os.environ.get("LEVOIT_TEST_SANITIZERS", "address,undefined"),
                    str(source),
                    "-o",
                    exe,
                ],
                check=True,
            )
            subprocess.run([exe], check=True)


if __name__ == "__main__":
    unittest.main()
