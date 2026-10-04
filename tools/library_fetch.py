"""Fetch selected historical public sources into build/references; never install them."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import ssl
import subprocess
import os
import tarfile
import urllib.request
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[1]
RELEASES = {
    "zlib-1.2.1": "https://zlib.net/fossils/zlib-1.2.1.tar.gz",
    "speex-1.2rc1": "https://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz",
    "libvorbis-1.0.1": "https://downloads.xiph.org/releases/vorbis/libvorbis-1.0.1.tar.gz",
}


class HTTPSRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        if urlparse(new_url).scheme.lower() != "https":
            raise ValueError("Download redirect must remain HTTPS")
        return super().redirect_request(request, response, code, message, headers, new_url)


def verify_source(archive, destination):
    """Refuse missing, modified or contaminated cached reference trees; never repair silently."""
    destination = Path(destination).resolve()
    expected = set()
    with tarfile.open(archive, "r:gz") as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            path = destination / member.name
            if not path.resolve().is_relative_to(destination) or not path.is_file() or path.is_symlink():
                raise ValueError("Cached source is missing or points outside its reference tree")
            expected.add(path.relative_to(destination).as_posix())
            with tar.extractfile(member) as source:
                if hashlib.file_digest(source, "sha256").digest() != hashlib.sha256(path.read_bytes()).digest():
                    raise ValueError("Cached source changed: " + member.name)
    actual = {path.relative_to(destination).as_posix() for path in destination.rglob("*") if path.is_file()}
    if actual != expected:
        raise ValueError("Cached source contains unexpected files")
    return len(expected)


def extract_source(archive, destination):
    """Copy regular files only; reject traversal, links, devices and oversized archives."""
    destination = Path(destination).resolve()
    with tarfile.open(archive, "r:gz") as tar:
        members = tar.getmembers()
        if len(members) > 10000 or sum(m.size for m in members) > 256 * 1024 * 1024:
            raise ValueError("Archive exceeds source extraction limits")
        for member in members:
            name = PurePosixPath(member.name)
            target = (destination / member.name).resolve()
            if name.is_absolute() or ".." in name.parts or "\\" in member.name or ":" in member.name or not target.is_relative_to(destination):
                raise ValueError("Unsafe archive path")
            if not (member.isfile() or member.isdir()):
                raise ValueError("Source archive contains a link or special file")
        for member in members:
            target = destination / member.name
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with tar.extractfile(member) as src, target.open("wb") as dst:
                    shutil.copyfileobj(src, dst)
    return len(members)


def fetch(release):
    url = RELEASES[release]
    directory = ROOT / "build" / "references" / release
    directory.mkdir(parents=True, exist_ok=True)
    metadata_path, archive = directory / "download.json", directory / (release + ".tar.gz")
    if metadata_path.exists():
        metadata = json.loads(metadata_path.read_text())
        if hashlib.sha256(archive.read_bytes()).hexdigest() != metadata["sha256"]:
            raise ValueError("Cached archive changed; refusing to reuse it")
        verify_source(archive, directory / "source")
        print(f"Using cached {release}")
        return metadata
    request = urllib.request.Request(url, headers={"User-Agent": "rb3-local-library-audit/1"})
    transport = "urllib"
    try:
        with urllib.request.build_opener(HTTPSRedirects()).open(request, timeout=30) as response:
            final_url = response.url
            data = response.read(32 * 1024 * 1024 + 1)
    except urllib.error.URLError as error:
        # Windows curl uses the OS certificate store; certificate checks remain enabled.
        if os.name != "nt" or not isinstance(error.reason, ssl.SSLCertVerificationError) or not shutil.which("curl.exe"):
            raise
        temporary = archive.with_suffix(".part")
        process = subprocess.run(["curl.exe", "--fail", "--location", "--silent", "--show-error",
            "--proto", "=https", "--proto-redir", "=https", "--max-time", "30", "--max-filesize", "33554432",
            "--output", str(temporary), "--write-out", "%{url_effective}", url], capture_output=True, text=True, timeout=40)
        if process.returncode:
            raise RuntimeError("Native HTTPS download failed: " + process.stderr)
        final_url, data = process.stdout.strip(), temporary.read_bytes()
        temporary.unlink()
        transport = "Windows curl with certificate validation"
    if urlparse(final_url).scheme != "https":
        raise ValueError("Download redirected away from HTTPS")
    if len(data) > 32 * 1024 * 1024:
        raise ValueError("Archive exceeds download limit")
    archive.write_bytes(data)
    extracted = directory / "source"
    if extracted.exists() and any(extracted.iterdir()):
        raise ValueError("Incomplete prior extraction exists; choose a fresh reference directory manually")
    count = extract_source(archive, extracted)
    metadata = {"release": release, "url": url, "resolved_url": final_url,
                "downloaded_at": datetime.now(timezone.utc).isoformat(), "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(), "archive_entries": count,
                "source": str(extracted / release), "hash_kind": "locally computed; not independently authenticated publisher checksum",
                "transport": transport, "adopted": False}
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(metadata, indent=2))
    return metadata


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release", choices=RELEASES)
    fetch(parser.parse_args().release)
