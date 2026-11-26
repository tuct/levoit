"""Native tests use synthetic frames, no credentials or device access."""

from pathlib import Path
import os
import subprocess
import tempfile
import unittest


class ProtocolTest(unittest.TestCase):
    def test_protocol(self):
        with tempfile.TemporaryDirectory() as temp:
            for test in ("protocol_test.cpp", "startup_test.cpp"):
                with self.subTest(test=test):
                    source = Path(__file__).with_name(test)
                    exe = str(Path(temp) / source.stem)
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
