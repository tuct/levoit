# Levoit Vital 200S Pro — Stock Firmware UART Analysis

## 1. Overview

Read-only analysis of the original Levoit ESP firmware shipped on the
Vital 200S Pro, performed to identify the UART command path used by the
device for sleep, quick-clean, white-noise, and daytime preference
writes. No code is executed; no UART writes are sent. The firmware
image, dispatcher table, and SET handler are documented below so a
future contributor extending the ESPHome component for a related Levoit
model can re-derive analogous findings without repeating the analysis.

**Firmware analyzed:** `VS_WFON_APR_LAP-V201S-AEUR_OFL_EU` v1.3.0-rc1,
compiled 2025-03-25, ESP-IDF dc489539. Backup file is 4,194,304 bytes,
starts with `0xE9` (ESP image magic).

**Headline findings:**

* The bulk-preferences SET is a single 12-TLV write at CMD `02 02 55`,
  local tags `0x04..0x0F`, emitted by the function at `0x420075a6`.
* Tag `0x04` (sleep_type) gates writes to tags `0x05..0x0F`: when
  `0x04 = 0x00`, the MCU silently drops the other 11 fields.
* The storage model is a single shared field set across all "preset"
  labels — not per-preset slots.
* Local-tag ↔ status-TLV mapping is monotonic:
  `set_tag = status_tlv − 0x14`.

---

## 2. UART frame structure

### 2.1 Frame envelope

Every ESP↔MCU frame uses the layout already documented in
[`LEVOIT_UART.md`](../LEVOIT_UART.md):

```
A5 22 <ctr> <len> 00 <chk> <cmd[0]> <cmd[1]> <cmd[2]> 00 <payload...>
```

`A5` is the start byte. `0x22` is the SEND message type for commands.
`<ctr>` is the per-direction sequence counter that increments per
frame. `<len>` is the total frame length. `<chk>` is the one-byte
checksum computed across the whole frame excluding byte index 5
(the checksum field itself).

### 2.2 The `klv_pack` helper (`0x4200b568`)

Every Vital command builder composes its payload via a shared
serialization helper. Reverse-engineered signature:

```c
size_t klv_pack(void   *buf,
                size_t  buf_size,
                uint8_t tag,
                int16_t len,
                const void *value_ptr);
//              a0      a1          a2          a3          a4
```

Behavior, derived from the disassembly at `0x4200b568..0x4200b630`:

| `len`        | Bytes written                                                  | Return value |
|--------------|----------------------------------------------------------------|--------------|
| 0            | `{tag, 0x00}` — 2 bytes, `value_ptr` ignored                   | 2            |
| 1..127       | `{tag, len, *memcpy(value_ptr, len)}` — `len + 2` bytes        | `len + 2`    |
| 128..32767   | `{tag, (len>>8) \| 0x80, len & 0xFF, value...}` long-form      | `len + 3`    |

If `buf == NULL`, the function returns the size that would have been
written without performing the copy. Callers chain calls by advancing
`buf` by the previous return value.

---

## 3. The UART send dispatcher (`0x42006e46`)

### 3.1 Calling convention

Every sender call site has the same shape:

```
<prepare TLV payload in sp+N>
c.lui   a0, 0x5
addi    a0, a0, <imm>        # a0 = 0x5XXX  ← key constant
jal     ra, 0x42006e46       # send_uart_dispatcher
c.beqz  a0, <skip_error>     # return value 0 = success
<log error>
```

The dispatcher accepts a 16-bit "key" in register `a0`. The key encodes
`(cmd[2] << 8) | cmd[1]`, with `cmd[0] = 0x02` implied (the Vital
command-family prefix).

### 3.2 Internal behavior

The dispatcher is **not a UART writer**. It assembles a 135-byte
heap-allocated message envelope plus a 24-byte routing struct on the
stack, then posts to a queue consumed by a downstream UART task:

```
0x42006e46(key=a0, a1, buf=a2, len=a3, a4):
  alloc 135-byte envelope (heap), zero-initialise
  write 24-byte routing struct on stack:
    struct[4]  = 1
    struct[8]  = key                       (e.g. 0x5505)
    struct[12] = buf  (TLV payload ptr)
    struct[16] = len  LE16
    struct[20] = envelope_ptr              (set later)
  envelope[0]    = a1 byte (caller-supplied context byte)
  if a4 != 0:
    envelope[1]  = 0x01
    memcpy(envelope[2..61], a4, 60)        ; 60 bytes of context
  memcpy(envelope[62..62+len-1], buf, len) ; TLV payload
  envelope[102]  = len byte (truncated to 1B)
  jal 0x420113ca(0, &struct)               ; post to message queue
```

`0x42021024` is a heap-alloc trampoline that tail-calls into
`0x40390e92` (IRAM). `0x420113ca` is a generic queue poster that hands
the routing struct off to the downstream UART task. The final wire
frame is constructed by that downstream task, not by the dispatcher.

### 3.3 Identified key → CMD mappings

The dispatcher key encoding has been verified against every key
constant emitted by an identified call site in the IROM segment:

| Key      | CMD bytes  | Feature                              | Call-site PCs                                  |
|----------|------------|--------------------------------------|------------------------------------------------|
| `0x5502` | `02 02 55` | fan mode + auto-mode prefs           | `0x420070b2`, `0x42007574`, `0x42007722`       |
| `0x5503` | `02 03 55` | fan speed                            | `0x42007120`                                   |
| `0x5504` | `02 04 55` | display                              | `0x42007198`                                   |
| `0x5511` | `02 11 55` | light detect                         | `0x42007796`                                   |
| `0x5140` | `02 40 51` | child lock                           | `0x42007322`                                   |
| `0x5141` | `02 41 51` | child-lock family variant            | `0x420073f2`                                   |
| `0x5104` | `02 04 51` | unmapped (filter-LED candidate)      | `0x4200745a`                                   |
| `0x5105` | `02 05 51` | unmapped                             | `0x420074e4`                                   |
| `0x5107` | `02 07 51` | unmapped                             | `0x420077d0`                                   |
| `0x5144` | `02 44 51` | unmapped                             | (not yet pinpointed)                           |
| `0x5501` | `02 01 55` | unmapped (single empty-payload call) | `0x42007818`                                   |
| `0x550a` | `02 0A 55` | Sprout-family overlap                | `0x4200793a`                                   |
| `0x5505` | `02 05 55` | orthogonal — see §7                  | `0x420072ac`, `0x42007388`                     |

The "fan mode + auto-mode prefs" key `0x5502` is also the key used by
the bulk-preferences SET (§4); the local-tag namespace under that CMD
is wide enough for multiple subsystems to coexist (§4.6).

---

## 4. Bulk-preferences SET — the 12-TLV write

For component-implementation use — the SET-tag → status-TLV mapping
in the format a decoder/builder needs — see
[`MCU_2.0.0_baseline.md` §4.2](./MCU_2.0.0_baseline.md#42-set-tag--status-tlv-mapping).

### 4.1 Overview

| Item                    | Value                                              |
|-------------------------|----------------------------------------------------|
| Function entry          | `0x420075a6`                                       |
| Dispatcher call site    | `0x42007726`                                       |
| Dispatcher key          | `0x5502`                                           |
| CMD bytes               | `02 02 55`                                         |
| Caller-supplied context | 33-byte struct (passed as first arg)               |
| Payload size            | 39 bytes                                           |
| Total wire frame        | 49 bytes (39 payload + 10-byte envelope)           |

### 4.2 The 12 `klv_pack` calls

The function packs twelve TLVs in this exact order, reading source
bytes from offsets inside the 33-byte input struct:

| # | Local tag | Length | Source (cfg offset) | Wire bytes                  |
|---|-----------|--------|---------------------|------------------------------|
| 1 | `0x04`    | 1      | `cfg[0]`            | `04 01 <b0>`                 |
| 2 | `0x05`    | 1      | `cfg[4]`            | `05 01 <b4>`                 |
| 3 | `0x06`    | 2 LE16 | `cfg[8..9]`         | `06 02 <b8> <b9>`            |
| 4 | `0x07`    | 1      | `cfg[10]`           | `07 01 <b10>`                |
| 5 | `0x08`    | 1      | `cfg[12]`           | `08 01 <b12>`                |
| 6 | `0x09`    | 2 LE16 | `cfg[16..17]`       | `09 02 <b16> <b17>`          |
| 7 | `0x0A`    | 1      | `cfg[18]`           | `0A 01 <b18>`                |
| 8 | `0x0B`    | 1      | `cfg[19]`           | `0B 01 <b19>`                |
| 9 | `0x0C`    | 2 LE16 | `cfg[20..21]`       | `0C 02 <b20> <b21>`          |
|10 | `0x0D`    | 1      | `cfg[24]`           | `0D 01 <b24>`                |
|11 | `0x0E`    | 1      | `cfg[28]`           | `0E 01 <b28>`                |
|12 | `0x0F`    | 1      | `cfg[32]`           | `0F 01 <b32>`                |

After the twelfth `klv_pack` returns, the function loads
`addi a0, a0, 1282 # 0x5502` and `jal 0x42006e46` to dispatch.

### 4.3 Wire-frame example

A complete bulk-write frame with `sleep_type = 1 (Custom1)` and the
other 11 fields at illustrative defaults:

```
A5 22 19 2B 00 F7 02 02 55 00
   04 01 01      ← sleep_type = 1 (gate unlocked)
   05 01 01      ← quick_clean_enabled
   06 02 05 00   ← quick_clean_minutes  (LE16 = 5)
   07 01 03      ← quick_clean_fan
   08 01 01      ← white_noise_enabled
   09 02 2D 00   ← white_noise_minutes  (LE16 = 45)
   0A 01 01      ← white_noise_fan
   0B 01 05      ← sleep_fan            (5 = auto)
   0C 02 E0 01   ← sleep_minutes        (LE16 = 480)
   0D 01 01      ← daytime_enabled
   0E 01 02      ← daytime_fan_mode
   0F 01 01      ← daytime_fan_level
```

`<len>` byte `0x2B` = 43 bytes total minus the 6-byte fixed framing
header equals the 39 payload bytes of klv_pack output.

### 4.4 Tag `0x04` gate behavior

Empirically verified by on-device writes against MCU FW 2.0.0:

* When the bulk-write frame contains `tag=0x04 value=0x00`, the MCU
  applies *only* the `0x04` change to status TLV `0x18`. Writes to tags
  `0x05..0x0F` are silently dropped: the MCU returns the standard
  `0x12` ACK and the subsequent status push shows the original values
  for the other 11 fields.
* When `tag=0x04 value != 0x00` (the observed non-zero values are
  `0x01` and `0x02`, corresponding to "Custom1" and "Custom2" in the
  stock app), the MCU applies all 12 writes.

Client implementations that expose the cluster fields as writable
entities must therefore either:

1. Pre-set `tag=0x04` to a non-zero value before writing any other
   cluster field, or
2. Surface the gate to the user (e.g. as a writable select) and
   document that other cluster edits do not persist while
   `tag=0x04 = 0x00`.

### 4.5 Storage model

The MCU exposes a single shared field set, **not per-preset slots**.
This is observable from the status push:

* Writing a non-Default `tag=0x04` followed by new values for tags
  `0x05..0x0F` updates the corresponding status TLVs `0x18..0x23`.
* Subsequently writing `tag=0x04 = 0x00` flips status TLV `0x18` to
  zero but leaves status TLVs `0x19..0x23` unchanged. Switching back
  to the same non-Default `tag=0x04` value does not restore any
  "remembered" per-preset values — TLVs `0x19..0x23` continue to
  reflect whatever was last written.

The `tag=0x04` byte is therefore an active-profile label, not a slot
selector. There is no per-preset storage within the firmware's
observable state.

### 4.6 Coexistence with `setAutoMode*` on the same CMD byte

The same CMD `02 02 55` is used by an older multi-TLV builder for
auto-mode preferences. Function entry `0x42007512`, dispatcher key
`0x5502`. Annotated disassembly of its klv_pack sequence:

```
;  --- function prologue: stash caller args ---
42007512:  c.addi16sp sp, -80
42007516:  c.swsp     a0, 12(sp)              ; sp+12 = auto_mode byte
4200751a:  sh         a1, 10(sp)              ; sp+10..sp+11 = room_size LE16

;  --- klv_pack #1: writes {0x02, 0x01, mode} ---
42007538:  c.addi4spn a4, sp, 12               ; value_ptr = sp+12
4200753a:  c.li       a3, 1                    ; len = 1
4200753c:  c.li       a2, 2                    ; tag = 0x02 (local namespace)
4200753e:  addi       a1, zero, 40             ; buf_size = 40
42007542:  c.addi4spn a0, sp, 24               ; buf = sp+24
42007544:  jal        0x4200b568               ; klv_pack

;  --- klv_pack #2: writes {0x03, 0x02, lo, hi} ---
42007556:  addi       a4, sp, 10               ; value_ptr = sp+10 (room_size)
4200755a:  c.li       a3, 2                    ; len = 2
4200755c:  c.li       a2, 3                    ; tag = 0x03 (local namespace)
4200755e:  c.add      a0, a5                   ; buf = original + first_write_size
42007560:  jal        0x4200b568               ; klv_pack

;  --- dispatcher call ---
4200756a:  c.lui      a0, 0x5
42007574:  addi       a0, a0, 1282 # 0x5502    ; key = 0x5502 → CMD 02 02 55
42007578:  jal        0x42006e46
```

The auto-pref builder writes local tags `0x02` and `0x03`; the
bulk-prefs builder (`0x420075a6`) writes local tags `0x04..0x0F`. The
two namespaces are disjoint, so both subsystems can share the same CMD
byte. The MCU's parser dispatches on tag IDs within the `02 02 55`
payload, applying changes selectively per tag.

This explains why an ESPHome component that already implements
`setAutoMode*` writes against `CMD 02 02 55` can add a bulk-preferences
SET against the same CMD byte without conflict — the local-tag
namespaces never overlap.

---

## 5. SET tag ↔ status TLV mapping

The 12 local tags in the bulk write correspond to the 12 status TLVs
that the MCU pushes for the four cluster subsystems. The mapping is
monotonic: **`set_tag = status_tlv − 0x14`**, with field lengths
matching exactly.

| SET tag | Length  | Status TLV | Field                       |
|---------|---------|------------|-----------------------------|
| `0x04`  | 1 B     | `0x18`     | sleep_type (gate; see §4.4) |
| `0x05`  | 1 B     | `0x19`     | quick_clean_enabled         |
| `0x06`  | LE16    | `0x1A`     | quick_clean_minutes         |
| `0x07`  | 1 B     | `0x1B`     | quick_clean_fan             |
| `0x08`  | 1 B     | `0x1C`     | white_noise_enabled         |
| `0x09`  | LE16    | `0x1D`     | white_noise_minutes         |
| `0x0A`  | 1 B     | `0x1E`     | white_noise_fan             |
| `0x0B`  | 1 B     | `0x1F`     | sleep_fan                   |
| `0x0C`  | LE16    | `0x20`     | sleep_minutes               |
| `0x0D`  | 1 B     | `0x21`     | daytime_enabled             |
| `0x0E`  | 1 B     | `0x22`     | daytime_fan_mode            |
| `0x0F`  | 1 B     | `0x23`     | daytime_fan_level           |

Cluster grouping (purely organisational, not enforced by the
protocol):

* Sleep: `0x04` (type) + `0x0B` (fan) + `0x0C` (minutes)
* Quick-clean: `0x05` (enabled) + `0x07` (fan) + `0x06` (minutes)
* White-noise: `0x08` (enabled) + `0x0A` (fan) + `0x09` (minutes)
* Daytime: `0x0D` (enabled) + `0x0E` (mode) + `0x0F` (level)

Within each cluster, the field order is `enabled / fan / minutes` for
the three audio-and-air clusters and `enabled / mode / level` for the
daytime cluster.

---

## 6. CMD `02 05 55` (key `0x5505`) — analyzed, orthogonal to preferences

This CMD was a candidate for the sleep-preferences SET before the
bulk-write at key `0x5502` was identified (§4). It is documented here
so a future contributor can recognise the channel without re-walking
the analysis.

For the gap-analysis context — how this CMD fits into the broader
inventory of MCU commands the component does not yet use — see
[`MCU_2.0.0_baseline.md` §5.2](./MCU_2.0.0_baseline.md#52-cmd-02-05-55--orthogonal-channel).

### 6.1 Site A — function `0x4200726e`, dispatcher call at `0x420072b0`

Builds a single-TLV payload then dispatches:

```
;  --- prologue ---
4200726e:  c.addi16sp sp, -80
42007274:  c.swsp     a0, 12(sp)              ; sp+12 = caller's a0 byte

;  --- klv_pack (only one call): writes {0x01, 0x01, byte} ---
4200728e:  c.addi4spn a4, sp, 12               ; value_ptr = sp+12 (the byte)
42007290:  c.li       a3, 1                    ; len = 1
42007292:  c.li       a2, 1                    ; tag = 0x01
42007294:  addi       a1, zero, 40
42007298:  c.addi4spn a0, sp, 24
4200729a:  jal        0x4200b568

;  --- dispatcher ---
420072a2:  c.lui      a0, 0x5
420072ac:  addi       a0, a0, 1285 # 0x5505    ; CMD 02 05 55
420072b0:  jal        0x42006e46
```

Wire bytes: `A5 22 <ctr> 07 00 <chk> 02 05 55 00 01 01 <byte>`.

### 6.2 Site B — function `0x42007354`, dispatcher call at `0x4200738c`

Builds a `{tag=0x03, len=0}` empty-value TLV — the conventional shape
of a query / "report this field" message in this firmware family:

```
;  --- klv_pack (only one call): writes {0x03, 0x00} — NO value ---
4200736a:  c.li       a4, 0                    ; value_ptr = NULL
4200736c:  c.li       a3, 0                    ; len = 0
4200736e:  c.li       a2, 3                    ; tag = 0x03
42007374:  c.addi4spn a0, sp, 8
42007376:  jal        0x4200b568

420737e:   c.lui      a0, 0x5
42007388:  addi       a0, a0, 1285 # 0x5505    ; CMD 02 05 55
4200738c:  jal        0x42006e46
```

Wire bytes: `A5 22 <ctr> 06 00 <chk> 02 05 55 00 03 00`.

### 6.3 Empirical behavior

Frames matching the site-A shape (single-byte write at local tag
`0x01`) sent on-device produce a clean `0x12` ACK from the MCU but no
change to any status TLV — including the 12 preference TLVs at
`0x18..0x23` and the surrounding diagnostic TLVs at `0x16/0x17`. The
MCU recognises the CMD byte and the frame, but the data is consumed by
a subsystem that does not surface state in the status push.

### 6.4 Hypothesis (based on call-site context, unverified)

The site-A function `0x4200726e` is invoked from two upstream callers
at PCs `0x420010ec` and `0x42001432`. Both callers:

* Increment a counter at DRAM `0x3fc9d7a4` immediately before the call.
* Memcpy a 20-byte struct in the same basic block.
* Live in a compilation unit whose DROM string-pool region contains
  the literals `Read air filter info fail`, `filter dust percent`,
  `filter use time = %d`, `Create filter timer fail!!!`, and
  `filter algorithm version = 0x%04x`.

The "increment counter → memcpy struct → emit single-byte report"
pattern matches a periodic filter-usage report channel, and the
surrounding filter-related string constants reinforce that
association. However: no caller of `0x4200726e` was traced to
completion, and the 20-byte struct contents were not extracted. The
"filter monitor channel" association is therefore a hypothesis based
on circumstantial call-site context, not on inspecting the data that
the site actually writes. Independent verification (e.g. capturing
this CMD's emission during a filter-life event on the stock firmware)
remains open.

---

## 7. Reproducing the analysis

### 7.1 Tools used

* `esptool image-info` — segment layout and entry-point
* `strings -n 5` — DROM symbol extraction
* `riscv32-esp-elf-objdump -D -b binary -m riscv:rv32 -M no-aliases
  --adjust-vma=0x42000020 /tmp/stock_irom.bin` — IROM disassembly
  (RV32 with no-aliases option suppresses pseudo-instruction
  expansion, making compiler-emitted patterns easier to grep)

### 7.2 ESP image segment layout

| Segment | Type             | Vaddr        | File offset | Length    |
|---------|------------------|--------------|-------------|-----------|
| 0       | DROM (`.rodata`) | `0x3C0F0020` | `0x10018`   | `0x21288` |
| 1       | DRAM             | `0x3FC95E00` | `0x312A8`   | `0x03178` |
| 2       | IRAM             | `0x40380000` | `0x34428`   | `0x0BBE8` |
| 3       | IROM (`.text`)   | `0x42000020` | `0x40018`   | `0xE2CA0` |
| 4       | IRAM             | `0x4038BBE8` | `0x122CC0`  | `0x0A1C4` |

Vaddrs match ESP32-C3 memory layout (DROM at `0x3C0x_xxxx`, DRAM at
`0x3FCx_xxxx`, IRAM at `0x4038_xxxx`, IROM at `0x42xx_xxxx`). The
RISC-V architecture confirms ESP32-C3 specifically (not the older
Xtensa-based ESP32).

### 7.3 Why byte-pattern search alone is unreliable

A literal grep for `02 XX 55` byte triples in the image is **not** a
reliable signal for identifying CMD-byte usage. The compiler emits
the CMD-bytes constants via paired `c.lui` + `addi` instructions
(loading a 12-bit immediate into the upper half of `a0`, then adding
the lower half) rather than storing them as literal byte triples.
Of all 32 possible `02 XX 55` triples in the image:

* `02 04 55` (display) and `02 11 55` (light detect) never appear as
  contiguous byte triples at all.
* `02 02 55` (auto pref) appears 2× — both in non-code data.

To locate a CMD usage, search the disassembly for the corresponding
`0x55XX` immediate following an `addi a0, a0, <imm>` after a
`c.lui a0, 0x5`, and walk backward through the call site to identify
the function entry.

### 7.4 Caveat: dispatcher call-site count is not a reliable identifier

Matching by dispatcher call-site count alone is unreliable. The
bulk-prefs SET at key `0x5502` has a single call site (`0x420075a6`),
whereas key `0x5505` has two (`0x4200726e` + `0x42007354`). A
superficial multiplicity heuristic — assuming a "set" handler should
appear at the same frequency as related "set" / "get" pairs elsewhere
in the dispatcher — would have favored the wrong key. Cross-reference
`klv_pack` signatures (§2.2) and string-pool symbols (§8.3) rather
than relying on call-site counts.

---

## 8. Address reference

### 8.1 Function entries

| PC           | Function                                                       |
|--------------|----------------------------------------------------------------|
| `0x42006e46` | UART send dispatcher (key in `a0`)                             |
| `0x4200b568` | `klv_pack` TLV serializer                                      |
| `0x42007512` | auto-pref multi-TLV builder (key `0x5502`, tags 0x02+0x03)     |
| `0x420075a6` | **bulk-preferences SET (key `0x5502`, tags 0x04..0x0F)**       |
| `0x4200726e` | `0x5505` site A — single-byte write at local tag `0x01`        |
| `0x42007354` | `0x5505` site B — empty-value query at local tag `0x03`        |
| `0x42021024` | heap-alloc trampoline (tail-calls to `0x40390e92` in IRAM)     |
| `0x420113ca` | message-queue poster (downstream of dispatcher)                |
| `0x40390e92` | heap allocator implementation (IRAM)                           |

### 8.2 Dispatcher call sites by key

(All keys reached via the `0x42006e46` dispatcher; repeated from §3.3
for navigation convenience.)

| Key      | Call-site PCs                                          |
|----------|--------------------------------------------------------|
| `0x5502` | `0x420070b2`, `0x42007574`, `0x42007722`               |
| `0x5503` | `0x42007120`                                           |
| `0x5504` | `0x42007198`                                           |
| `0x5511` | `0x42007796`                                           |
| `0x5140` | `0x42007322`                                           |
| `0x5141` | `0x420073f2`                                           |
| `0x5104` | `0x4200745a`                                           |
| `0x5105` | `0x420074e4`                                           |
| `0x5107` | `0x420077d0`                                           |
| `0x5501` | `0x42007818`                                           |
| `0x5505` | `0x420072ac` (site A), `0x42007388` (site B)           |
| `0x550a` | `0x4200793a`                                           |

### 8.3 DROM string-pool symbols

Identified via `strings -n 5` on segment 0. The function entries
backing the named symbols were **not** all pinpointed; the symbols
themselves are reference markers in the data segment.

| Symbol                                  | File offset | Vaddr        |
|-----------------------------------------|-------------|--------------|
| `sleepPreferenceType`                   | 72,904      | `0x3C0F1CD0` |
| `duringSleepSpeedLevel`                 | 73,480      | `0x3C0F1F10` |
| `duringSleepMinutes`                    | 73,572      | `0x3C0F1F6C` |
| `purifier_bypass_set_sleep_preference`  | 74,308      | `0x3C0F224C` |
| `setSleepPreference`                    | 74,524      | `0x3C0F2324` |
| `alterSleepModePreference`              | 76,136      | `0x3C0F2970` |
| `enterSleepMode`                        | 76,180      | `0x3C0F299C` |
| `purifier_uart_set_sleep_preference`    | 79,120      | `0x3C0F3518` |
| `insleep_minutes`                       | 81,600      | `0x3C0F3EC8` |
| `insleep_fanlevel`                      | 81,632      | `0x3C0F3EE8` |
| `alterAutoModePreference`               | 76,100      | `0x3C0F294C` |
| `setAutoPreference`                     | 74,440      | `0x3C0F22D0` |
| `purifier_uart_set_auto_preference`     | 78,840      | `0x3C0F3400` |

GCC's `-fmerge-strings` optimisation pools shorter strings as suffixes
of longer ones in this image, so multiple `ESP_LOG` sites can
legitimately reference mid-string offsets that happen to land inside
larger symbols used elsewhere. Symbol presence alone does not identify
the function — the byte-store sequence at each call site is the
authoritative signal.

---

## 9. Open uncertainties

* The dispatcher (`0x42006e46`) downstream of its queue post was not
  fully traced. The 24-byte routing struct it produces is consumed by
  a different task, and the wire-frame assembly logic on that side
  was not disassembled. Frames produced by the ESPHome component's
  `build_levoit_message()` helper are byte-identical to what the stock
  firmware emits for every
  identified key (verified empirically against fan / auto / display /
  light-detect / child-lock writes), which strongly suggests the wire
  shape is fully described by the key + payload pair; but the
  downstream task is not reverse-engineered here.
* The "single shared store" conclusion for the bulk-prefs storage
  model (§4.5) is based on observable status push state across
  consecutive writes, not on locating and reading the underlying RAM
  data structure. An exhaustive scan of MCU RAM around the time of
  preset switches could surface per-preset slots that aren't reflected
  in the status push; this has not been done.
* CMD `02 05 55` (key `0x5505`, §6) is documented as orthogonal to the
  preference subsystem but the actual subsystem it serves is a
  hypothesis based on call-site context (§6.4). Confirming it via
  capture during a stock-firmware filter-life event would close this
  open question.
* Keys `0x5104`, `0x5105`, `0x5107`, `0x5144`, `0x5501`, `0x550a`
  are observed in the dispatcher table but the functions that
  populate their payloads were not traced. Each represents a small
  amount of additional reverse-engineering work for a contributor
  interested in mapping unused MCU features.

---

## Footer

| Item                            | Value                                                  |
|---------------------------------|--------------------------------------------------------|
| Stock ESP firmware              | `VS_WFON_APR_LAP-V201S-AEUR_OFL_EU` v1.3.0-rc1         |
| Stock firmware compiled         | 2025-03-25                                             |
| Stock firmware ESP-IDF          | dc489539                                               |
| SoC architecture                | ESP32-C3 (RISC-V RV32, single-core)                    |
| MCU firmware verified against   | 2.0.0                                                  |
| Protocol verification date      | 2026-05-20                                             |
| Compiler version                | not directly observable from the image                 |
