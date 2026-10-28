#!/usr/bin/env python3
"""Static repository and firmware-identity checks.

POWERED BY [mikey:hexoid-sys]
Created by mikey-7x — Yogesh R. Chauhan
Author GitHub: https://github.com/mikey-7x

These checks protect the public release from branding loss, broken
local documentation links, accidental reintroduction of release labels
and incorrect range/generator claims.  They complement, and do not
replace, Arduino compilation and physical hardware testing.
"""

from __future__ import annotations

import base64
import hashlib
from html.parser import HTMLParser
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CHECKS: list[tuple[str, bool]] = []


def check(name: str, condition: object) -> None:
    CHECKS.append((name, bool(condition)))


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


COUNTER_PATH = "firmware/frequency_counter/frequency_counter.ino"
GENERATOR_PATH = "firmware/frequency_generator/frequency_generator.ino"
counter = text(COUNTER_PATH)
generator = text(GENERATOR_PATH)
readme = text("README.md")
license_text = text("LICENSE")
notice = text("NOTICE.md")
logo_svg = text("assets/powered-by-mikey-hexoid-sys.svg")
metadata_text = text("project-metadata.json")
citation_text = text("CITATION.cff")
authors_text = text("AUTHORS.md")
repository_url = (
    "https://github.com/mikey-7x/"
    "298.666mhz-frequency-counter-using-stm32f103c8t6-and-"
    "vhf-uhf-fm-transmitter-circuit"
)
project_title = (
    "298.666mhz frequency counter using stm32f103c8t6 & "
    "vhf/uhf fm transmitter circuit"
)
powered_by = "POWERED BY [mikey:hexoid-sys]"

# Repository structure and sketch naming.
for path in (
    COUNTER_PATH,
    GENERATOR_PATH,
    "hardware/complete-circuit-diagram.png",
    "hardware/complete-circuit-diagram-enhanced.png",
    "assets/project-title-logo.png",
    "assets/README.md",
    "assets/powered-by-mikey-hexoid-sys.svg",
    "assets/powered-by-mikey-hexoid-sys.png",
    "assets/mikey-hexoid-sys-mark.svg",
    "assets/mikey-hexoid-sys-mark.png",
    "assets/github-social-preview.png",
    "AUTHORS.md",
    "CITATION.cff",
    "CODEOWNERS",
    "CONTRIBUTING.md",
    "SECURITY.md",
    ".github/pull_request_template.md",
    "docs/theory-and-limits.md",
    "docs/build-and-operation.md",
    "docs/mikey-hexoid-and-zflasher.md",
    "incoming-counter-build/README.md",
    "incoming-counter-build/SHA256SUMS",
    "incoming-counter-build/v23gen2.hex",
    "incoming-counter-build/v23gen2.bin",
    "incoming-counter-build/v23gen2.elf",
    "incoming-counter-build/v23gen2.map",
    "tools/verify_precompiled.py",
    "docs/transmitter-notes.md",
    "docs/validation-checklist.md",
    "docs/build-results.txt",
    "docs/test-record.csv",
):
    check(f"required file exists: {path}", (ROOT / path).is_file())

for path in (COUNTER_PATH, GENERATOR_PATH):
    sketch = ROOT / path
    check(
        f"Arduino folder matches sketch basename: {path}",
        sketch.parent.name == sketch.stem,
    )

# Identity and licence notices embedded in both firmware files.
for label, source in (("counter", counter), ("generator", generator)):
    check(f"{label} contains exact project title", project_title in source)
    check(f"{label} contains exact powered-by line", powered_by in source)
    check(f"{label} credits mikey-7x", '#define HEXOID_AUTHOR "mikey-7x"' in source)
    check(
        f"{label} credits Yogesh R. Chauhan",
        '#define HEXOID_AUTHOR_LEGAL_NAME "Yogesh R. Chauhan"' in source
        and '#define HEXOID_AUTHOR_DISPLAY "Yogesh R. Chauhan (mikey-7x)"' in source,
    )
    check(
        f"{label} contains GitHub profile and repository URLs",
        '#define HEXOID_GITHUB_PROFILE_URL "https://github.com/mikey-7x"' in source
        and repository_url in source,
    )
    check(
        f"{label} contains YouTube and blog URLs",
        "https://youtube.com/@mikey-7x" in source
        and "https://rtod-yv8.blogspot.com/?m=1" in source,
    )
    check(
        f"{label} contains full copyright holder",
        "Copyright (c) 2026 Yogesh R. Chauhan (mikey-7x). All rights reserved."
        in source,
    )
    check(
        f"{label} names strict licence",
        "mikey-7x Strict Personal & Educational Use Licence" in source,
    )
    check(
        f"{label} anchors embedded identity string",
        "HEXOID_FIRMWARE_IDENTITY" in source
        and "volatile const char *hexoidIdentityAnchor" in source
        and "hexoidIdentityAnchor=HEXOID_FIRMWARE_IDENTITY" in source,
    )
    check(
        f"{label} contains no two-digit release label",
        re.search(r"\bV[0-9]{2}\b", source) is None,
    )

# Startup artwork is embedded and the detailed self-test text remains present.
check("counter includes one-bit startup logo", "OLED_LOGO_BITMAP[128u * 8u]" in counter)
check("startup logo duration is configurable", "#define LOGO_SPLASH_MS 3000u" in counter)
check("counter preserves self-test screen", 'startupSelfTestOK ? "SELF PASS" : "SELF FAIL"' in counter)
check("startup presentation can retry after OLED recovery", "startupArmed && startupStage < 2u" in counter and "return showPendingStartup();" in counter)
check("startup logo stage completes only after a successful frame", "if (!pushScreen()) return false;" in counter and "startupStage=1u;" in counter)
check("startup self-test stage completes only after a successful frame", "startupStage=2u;" in counter)
check("README links FM-radio demonstration", "https://youtu.be/2F3CqLfh4eI" in readme)
check("README links author build tool", "https://github.com/mikey-7x/mikey-hexoid-v5.0.0/tree/main" in readme)
check("README prominently credits the compilation tool", "**Compilation tool by mikey-7x:**" in readme and "| Compilation tool: mikey:hexoid v5.0.0 |" in readme)
check("README links Android flashing guide", "docs/mikey-hexoid-and-zflasher.md" in readme)
check("workflow verifies precompiled artifacts", "python3 tools/verify_precompiled.py" in text(".github/workflows/firmware-builds.yml"))
check("README displays actual OLED startup photo", 'src="docs/oled-startup-photo.jpg"' in readme)
check("OLED photo is a JPEG", (ROOT / "docs/oled-startup-photo.jpg").is_file() and (ROOT / "docs/oled-startup-photo.jpg").read_bytes().startswith(b"\xff\xd8\xff"))
check("build guide links OLED startup photo", "oled-startup-photo.jpg" in text("docs/build-and-operation.md"))

# Counter architecture invariants.
for name, needle in (
    ("counter defaults to 112 MHz", "#define COUNTER_CLOCK_MHZ 112"),
    ("counter calibration defaults to zero", "#define CAL_PPM 0.0"),
    ("counter derives design ceiling", "designMaxHz=2.0*(double)timerHz"),
    ("counter derives sync ceiling", "displayMaxHz=8.0*(double)timerHz/3.0"),
    ("counter uses dynamic over-range threshold", "else if (hz>displayMaxHz)"),
    ("counter marks experimental MHz", '"MHz EXP"'),
    ("counter retains hardware chain snapshot", "static uint32_t readCount32()"),
    ("counter gate sleeps between snapshots", "__WFI()"),
    ("counter has 100 ms tracking label", '"ETR/8 100ms TRACK"'),
    ("counter has 250 ms precision label", '"ETR/8 250ms GATE"'),
    ("counter OLED brand is mikey-7x", '"mikey-7x COUNTER"'),
    ("counter OLED powered-by line split is exact", '"[mikey:hexoid-sys]"'),
    ("counter disconnected state is present", '"NO SIGNAL"'),
):
    check(name, needle in counter)

# Generator exact-step invariants.
check("generator normal sweep starts at 1 Hz", "steps[]={1,10,100,1000" in generator)
check("generator normal sweep reaches 36 MHz", "36000000};" in generator)
check(
    "generator optional sweep reaches exact 64 MHz",
    "40000000,48000000,56000000,64000000" in generator,
)
check("generator does not contain a 65 MHz step", "65000000" not in generator)
check("generator RAM-isolated step is retained", "ramRun64Step" in generator)
check("generator button pin remains PB12", "#define BUTTON_PIN PB12" in generator)

# Range arithmetic.
def design_ceiling(timer_hz: int) -> int:
    return 2 * timer_hz


def display_ceiling(timer_hz: int) -> float:
    return 8 * timer_hz / 3


for timer_mhz, design_mhz, display_mhz in (
    (72, 144, 192.0),
    (96, 192, 256.0),
    (112, 224, 298.6666667),
):
    timer_hz = timer_mhz * 1_000_000
    check(
        f"{timer_mhz} MHz design ceiling is {design_mhz} MHz",
        design_ceiling(timer_hz) == design_mhz * 1_000_000,
    )
    check(
        f"{timer_mhz} MHz display ceiling is {display_mhz} MHz",
        abs(display_ceiling(timer_hz) / 1_000_000 - display_mhz) < 0.000001,
    )

# Recorded raw-count arithmetic.
check("recorded 182 MHz count decodes correctly", 22_858_903 * 8 == 182_871_224)
check("recorded 250 ms high count decodes correctly", 8_572_621 * 8 / 0.25 == 274_323_872)
check("recorded 1 s high count decodes correctly", 34_309_220 * 8 == 274_473_760)
check("GHz input would need 375 MHz by /3", 3 * (1_000_000_000 // 8) == 375_000_000)
check("GHz input would need 500 MHz by /4", 4 * (1_000_000_000 // 8) == 500_000_000)

# Documentation and legal identity.
for label, source in (
    ("README", readme),
    ("NOTICE", notice),
    ("LICENSE", license_text),
):
    check(f"{label} contains powered-by line", powered_by in source)
    check(f"{label} credits mikey-7x", "mikey-7x" in source)
    check(f"{label} credits Yogesh R. Chauhan", "Yogesh R. Chauhan" in source)
    check(f"{label} links official repository", repository_url in source)

for phrase in (
    "Personal Use",
    "Educational Use",
    "ABSOLUTE COMMERCIAL AND INDUSTRIAL PROHIBITION",
    "MANDATORY CREDIT, LOGO AND NOTICES",
    "separate written commercial licence signed by the Author",
    "NOT an\nopen-source licence",
):
    check(f"licence contains: {phrase!r}", phrase in license_text)

check("wide SVG title is exact", f"<title id=\"title\">{powered_by}</title>" in logo_svg)
check("wide SVG wordmark is exact", ">[mikey:hexoid-sys]</text>" in logo_svg)
check("README uses wide PNG logo", "assets/powered-by-mikey-hexoid-sys.png" in readme)
check("README uses original circuit diagram", "hardware/complete-circuit-diagram.png" in readme)
check("README corrects generator top step to 64 MHz", "exact top TIM1 step is **64 MHz**" in readme)
check("README contains author identity notice", "online pseudonyms of **Yogesh R. Chauhan**" in readme)
check("README links author catalogue", "https://github.com/mikey-7x?tab=repositories" in readme)
check("README links author YouTube", "https://youtube.com/@mikey-7x" in readme)
check("README links author blog", "https://rtod-yv8.blogspot.com/?m=1" in readme)
check("metadata parses and identifies author", '"author_legal_name": "Yogesh R. Chauhan"' in metadata_text)
check("metadata identifies canonical repository", repository_url in metadata_text)
check("citation identifies author and alias", 'given-names: "Yogesh R."' in citation_text and 'alias: "mikey-7x"' in citation_text)
check("authors file contains required credit", "Created by mikey-7x — Yogesh R. Chauhan" in authors_text)
check("CODEOWNERS assigns repository to mikey-7x", "* @mikey-7x" in text("CODEOWNERS"))

# Verify the actual image placements and all available logo references.
for image in ("project-title-logo.png", "mikey-hexoid-sys-mark.png", "github-social-preview.png"):
    check(f"README displays {image}", f'src="assets/{image}"' in readme)
for asset in ("project-title-logo.png", "powered-by-mikey-hexoid-sys.png",
              "powered-by-mikey-hexoid-sys.svg", "mikey-hexoid-sys-mark.png",
              "mikey-hexoid-sys-mark.svg", "github-social-preview.png"):
    check(f"README references {asset}", f"assets/{asset}" in readme)
asset_readme_path = ROOT / "assets/README.md"
asset_readme = asset_readme_path.read_text(encoding="utf-8") if asset_readme_path.is_file() else ""
check("assets README describes brand assets", asset_readme.startswith("# Brand assets\n"))
check("assets README lists project-title logo", "`project-title-logo.png`" in asset_readme)

# PNG header verifies an alpha-capable RGBA image, not the shape of the mask.
for name in ("project-title-logo.png", "powered-by-mikey-hexoid-sys.png",
             "mikey-hexoid-sys-mark.png", "github-social-preview.png"):
    try:
        data = (ROOT / "assets" / name).read_bytes()
        alpha = len(data) >= 26 and data[:8] == b"\x89PNG\r\n\x1a\n" and data[25] == 6
    except OSError:
        alpha = False
    check(f"{name} is an RGBA PNG", alpha)

# The two SVGs are image-backed containers and must embed their matching PNG.
for stem in ("powered-by-mikey-hexoid-sys", "mikey-hexoid-sys-mark"):
    try:
        svg = ET.parse(ROOT / "assets" / (stem + ".svg")).getroot()
        image = svg.find("{http://www.w3.org/2000/svg}image")
        uri = image.get("{http://www.w3.org/1999/xlink}href", "") if image is not None else ""
        prefix = "data:image/png;base64,"
        matches = uri.startswith(prefix) and base64.b64decode(uri[len(prefix):], validate=True) == (ROOT / "assets" / (stem + ".png")).read_bytes()
    except (OSError, ValueError, ET.ParseError):
        matches = False
    check(f"{stem}.svg embeds matching PNG", matches)

# Catch stale recorded hashes. Update docs/build-results.txt whenever a
# listed file changes; these checks do not claim to reproduce firmware builds.
checksum_lines = re.findall(r"(?m)^([0-9a-f]{64})  ([^\r\n]+)$", text("docs/build-results.txt"))
expected_paths = {COUNTER_PATH, GENERATOR_PATH, "LICENSE", "README.md", "NOTICE.md",
                  "assets/powered-by-mikey-hexoid-sys.svg", "hardware/complete-circuit-diagram.png",
                  "docs/oled-startup-photo.jpg"}
check("checksum record contains expected paths once", len(checksum_lines) == len(expected_paths) and {p for _, p in checksum_lines} == expected_paths)
for digest, path in checksum_lines:
    try:
        valid = path in expected_paths and hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest
    except OSError:
        valid = False
    check(f"recorded SHA-256 matches: {path}", valid)

# Parse README HTML links as well as Markdown; GitHub displays images from HTML.
class ReferenceParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.refs = []
    def handle_starttag(self, tag, attrs):
        for name, value in attrs:
            if name in ("src", "href") and value:
                self.refs.append(value)

parser = ReferenceParser()
parser.feed(readme)
for ref in parser.refs:
    uri = urlsplit(ref)
    if uri.scheme or uri.netloc or not uri.path or uri.path.startswith("/"):
        continue
    check(f"README HTML target exists: {ref}", (ROOT / unquote(uri.path)).is_file())

# Local Markdown links in public docs.  External links and same-page
# anchors are intentionally ignored.
link_pattern = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
for doc in ROOT.rglob("*.md"):
    if ".git" in doc.parts:
        continue
    doc_text = doc.read_text(encoding="utf-8")
    for raw_link in link_pattern.findall(doc_text):
        link = raw_link.strip().split()[0].strip("<>")
        if not link or link.startswith(("#", "http://", "https://", "mailto:")):
            continue
        link_path = link.split("#", 1)[0]
        target = (doc.parent / link_path).resolve()
        check(f"local link from {doc.relative_to(ROOT)}: {link}", target.exists())

failed = [name for name, ok in CHECKS if not ok]
print(f"Repository verification: {len(CHECKS) - len(failed)}/{len(CHECKS)} passed")
for name, ok in CHECKS:
    print(f"{'PASS' if ok else 'FAIL'}  {name}")

if failed:
    print("\nFailed checks:", file=sys.stderr)
    for name in failed:
        print(f"- {name}", file=sys.stderr)
    raise SystemExit(1)
