from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting

MSG_TYPES = {0x22: "SEND", 0x12: "ACKN", 0x52: "RESP"}


class LevoitExtractor(HighLevelAnalyzer):
    supported_analyzers = ["Async Serial"]
    channel = ChoicesSetting(label="Channel", choices=("ESP->MCU", "MCU->ESP", "-"))
    include_raw_bytes = ChoicesSetting(label="Include Raw Bytes", choices=("No", "Yes"))
    result_types = {"packet": {"format": "{{data.text}}"}}

    def __init__(self):
        self.reset()

    def reset(self):
        self.waiting = True
        self.bytes = []
        self.expected_length = None
        self.start_time = None
        self.end_time = None

    def decode(self, frame):
        try:
            return self._decode(frame)
        except Exception as e:
            return AnalyzerFrame("packet", frame.start_time, frame.end_time, {"text": f"ERR: {e}"})

    def _decode(self, frame):
        if frame.type not in ("data", "byte"):
            return None

        byte = frame.data["data"][0]

        if self.waiting:
            if byte == 0xA5:
                self.waiting = False
                self.bytes = [byte]
                self.start_time = frame.start_time
                self.end_time = frame.end_time
            return None

        if byte == 0xA5 and len(self.bytes) < 6:
            self.bytes = [byte]
            self.start_time = frame.start_time
            self.end_time = frame.end_time
            self.expected_length = None
            return None

        self.bytes.append(byte)
        self.end_time = frame.end_time

        if len(self.bytes) == 4:
            self.expected_length = 6 + self.bytes[3]

        if (self.expected_length and len(self.bytes) >= self.expected_length) or len(self.bytes) > 512:
            result = self.emit_frame()
            self.reset()
            return result

        return None

    def emit_frame(self):
        if self.start_time is None or len(self.bytes) < 6:
            return None

        b = self.bytes
        msg_type = f"{MSG_TYPES.get(b[1], 'UNK')}(0x{b[1]:02X})"
        cmd = " ".join(f"{x:02X}" for x in b[6:9])
        payload = " ".join(f"{x:02X}" for x in b[10:]) if len(b) > 10 else ""

        raw_str = " ".join(f"{x:02X}" for x in b)
        raw = f" | RAW={raw_str}" if self.include_raw_bytes == "Yes" else ""
        ch = f"[{self.channel}] " if self.channel and self.channel != "-" else ""
        text = f"{ch}{msg_type} |  CMD={cmd}  |  PAY={payload}{raw}"

        return AnalyzerFrame("packet", self.start_time, self.end_time, {"text": text})
