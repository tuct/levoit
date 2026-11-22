#!/usr/bin/env python3
"""List the stock firmware of the devices in your VeSync account, with download URLs.

Based on fetch_ota_info.py from nico202/levoit-core300s-ota-wifi-flash (MIT).
Differences: it reads the raw device list, so models pyvesync does not know yet
(e.g. the Eleva 300X) are included, and it checks the firmware file server for
the installed versions, which finds the URLs even when no update is pending.

What it does, all read-only:
  1. logs in to the VeSync cloud with your account (pyvesync)
  2. reads your device list           /cloud/v1/deviceManaged/devices
  3. reads the firmware info          /cloud/v2/deviceManaged/getFirmwareUpdateInfoList
  4. sends HEAD requests to http://fw.vesync.com for the installed versions

The printed output contains no e-mail, cid or MAC address and can be posted
publicly. Nothing is stored unless you pass --json FILE (that file DOES contain
your device IDs; don't share it).

Usage:
    pip install pyvesync
    python vesync_firmware_info.py              # asks for e-mail, password, country
    VESYNC_USERNAME=… VESYNC_PASSWORD=… VESYNC_COUNTRY=DE python vesync_firmware_info.py
"""
from __future__ import annotations

import argparse
import asyncio
import getpass
import json
import os
import re
import sys
import urllib.error
import urllib.request
from dataclasses import MISSING, fields

from pyvesync import VeSync
from pyvesync.models.vesync_models import RequestDeviceListModel, RequestFirmwareModel
from pyvesync.utils.helpers import Helpers

FW_HOST = "http://fw.vesync.com/firmwareUpload"
VERSION = re.compile(r"^\d+\.\d+\.\d+$")


def head(url: str) -> int | None:
    """HTTP status of a HEAD request; the file server answers 403 for missing files."""
    try:
        with urllib.request.urlopen(urllib.request.Request(url, method="HEAD"), timeout=15) as r:
            return r.status
    except urllib.error.HTTPError as e:
        return e.code
    except OSError:
        return None


def candidate_urls(module: str, dtype: str, plugin: str, version: str) -> list[str]:
    """File names seen so far for the firmwareUpload/<configModule>/<plugin>/v1/ scheme."""
    if plugin == "mainFw":
        names = [f"{dtype}_WIFI_C1_ota_v{version}.rel.bin"]
    elif plugin == "mcuFw":
        names = [f"{dtype}_MCU_C1_ota_v{version}.bin"]
    else:
        names = [f"{dtype}_{plugin}_ota_v{version}.bin"]
    return [f"{FW_HOST}/{module}/{plugin}/v1/{n}" for n in names]


async def run(user: str, pw: str, country: str, json_path: str | None) -> int:
    async with VeSync(user, pw, country_code=country, redact=True) as m:
        if not await m.login():
            print("login failed (wrong password, or try another country code, e.g. US)")
            return 1
        req = RequestDeviceListModel(token=m.auth.token, accountID=m.auth.account_id,
                                     timeZone=m.time_zone)
        devs, _ = await m.async_call_api("/cloud/v1/deviceManaged/devices", "post",
                                         headers=Helpers.req_header_bypass(),
                                         json_object=req.to_dict())
        dev_list = ((devs or {}).get("result") or {}).get("list") or []

        body_fields = tuple(f.name for f in fields(RequestFirmwareModel)
                            if f.default_factory is MISSING and f.default is MISSING)
        body = Helpers.get_manager_attributes(m, body_fields)
        body["cidList"] = [d["cid"] for d in dev_list if d.get("cid")]
        fw, _ = await m.async_call_api("/cloud/v2/deviceManaged/getFirmwareUpdateInfoList",
                                       "post", json_object=RequestFirmwareModel(**body))
        fw_by_cid = {item.get("deviceCid"): item.get("firmUpdateInfos") or []
                     for item in (((fw or {}).get("result") or {}).get("cidFwInfoList") or [])}

        print(f"{len(dev_list)} device(s) in the account\n")
        for d in dev_list:
            module, dtype = d.get("configModule") or "", d.get("deviceType") or ""
            print(f"{d.get('deviceName')}")
            print(f"  deviceType:   {dtype}")
            print(f"  configModule: {module}")
            print(f"  firmware (device list): {d.get('currentFirmVersion')}")
            for fi in fw_by_cid.get(d.get("cid"), []):
                plugin = fi.get("pluginName")
                cur, latest = fi.get("currentVersion"), fi.get("latestVersion")
                print(f"  {plugin}: installed {cur}, latest {latest}")
                if fi.get("latestVersionUrl"):
                    print(f"    update URL (from the cloud): {fi['latestVersionUrl']}")
                for ver in dict.fromkeys(v for v in (cur, latest) if v and VERSION.match(v)):
                    for url in candidate_urls(module, dtype, plugin, ver):
                        status = head(url)
                        mark = "found" if status == 200 else f"not found ({status})"
                        print(f"    v{ver}: {mark}  {url}")
            print()

        if json_path:
            with open(json_path, "w", encoding="utf-8") as fh:
                json.dump({"devices": devs, "firmware": fw}, fh, indent=2, ensure_ascii=False)
            print(f"raw responses (contain your device IDs, don't share): {json_path}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json", metavar="FILE", help="also save the raw cloud responses")
    args = ap.parse_args()
    user = os.environ.get("VESYNC_USERNAME") or input("VeSync e-mail: ")
    pw = os.environ.get("VESYNC_PASSWORD") or getpass.getpass("VeSync password: ")
    country = os.environ.get("VESYNC_COUNTRY") or input("Country code of the account [US]: ") or "US"
    return asyncio.run(run(user.strip(), pw, country.strip().upper(), args.json))


if __name__ == "__main__":
    raise SystemExit(main())
