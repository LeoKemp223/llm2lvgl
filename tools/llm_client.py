"""Unified OpenAI API client for LVGL code generation and refinement."""

import base64
import json
import os
import re
import time
from pathlib import Path

import httpx

SETTINGS_PATH = Path(__file__).resolve().parent.parent / "workspace" / ".llm_settings.json"


def load_settings() -> dict:
    """Load settings from config file; return empty dict if missing."""
    if SETTINGS_PATH.is_file():
        try:
            return json.loads(SETTINGS_PATH.read_text("utf-8"))
        except (json.JSONDecodeError, OSError):
            return {}
    return {}


def _get_config() -> tuple[str, str, str | None]:
    """Return (api_key, model, base_url) from settings file or env vars."""
    settings = load_settings()
    api_key = settings.get("api_key") or os.environ.get("OPENAI_API_KEY", "")
    if not api_key:
        raise RuntimeError(
            "OPENAI_API_KEY not set. Configure it in Web UI settings or set the environment variable."
        )
    model = settings.get("model") or os.environ.get("OPENAI_MODEL", "gpt-4o")
    base_url = settings.get("base_url") or os.environ.get("OPENAI_BASE_URL") or None
    return api_key, model, base_url


def _default_model() -> str:
    settings = load_settings()
    return settings.get("model") or os.environ.get("OPENAI_MODEL", "gpt-4o")


def chat(
    messages: list,
    model: str | None = None,
    temperature: float = 0.2,
    max_tokens: int = 8192,
) -> str:
    """Send a chat completion request via raw httpx (bypasses SDK User-Agent filtering)."""
    api_key, default_model, base_url = _get_config()
    model = model or default_model
    url = (base_url or "https://api.openai.com/v1").rstrip("/") + "/chat/completions"

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {api_key}",
    }
    body = {
        "model": model,
        "messages": messages,
        "temperature": temperature,
        "max_tokens": max_tokens,
        "stream": True,
    }

    last_err: Exception | None = None
    for attempt in range(5):
        try:
            with httpx.Client(timeout=httpx.Timeout(connect=30, read=300, write=30, pool=30)) as client:
                with client.stream("POST", url, json=body, headers=headers) as resp:
                    if resp.status_code != 200:
                        resp.read()
                        status = resp.status_code
                        # Don't retry client errors (except 429 rate-limit)
                        if 400 <= status < 500 and status != 429:
                            raise RuntimeError(
                                f"HTTP {status} (non-retryable): {resp.text[:300]}"
                            )
                        raise RuntimeError(f"HTTP {status}: {resp.text[:300]}")
                    chunks: list[str] = []
                    total_chars = 0
                    for line in resp.iter_lines():
                        if not line.startswith("data: "):
                            continue
                        payload = line[6:]
                        if payload.strip() == "[DONE]":
                            break
                        data = json.loads(payload)
                        choices = data.get("choices", [])
                        if choices:
                            delta = choices[0].get("delta", {})
                            content = delta.get("content")
                            if content:
                                chunks.append(content)
                                total_chars += len(content)
                                print(f"\r[llm] generating... {total_chars} chars", end="", flush=True)
                    if total_chars:
                        print(f"\r[llm] done, {total_chars} chars        ", flush=True)
                    return "".join(chunks)
        except RuntimeError as exc:
            if "non-retryable" in str(exc):
                raise
            last_err = exc
            wait = min(2 ** attempt, 30)
            print(f"[llm_client] attempt {attempt+1}/5 failed: {exc}, retrying in {wait}s...", flush=True)
            if attempt < 4:
                time.sleep(wait)
        except Exception as exc:
            last_err = exc
            wait = min(2 ** attempt, 30)
            print(f"[llm_client] attempt {attempt+1}/5 failed: {exc}, retrying in {wait}s...", flush=True)
            if attempt < 4:
                time.sleep(wait)
    raise RuntimeError(f"OpenAI API failed after 5 attempts: {last_err}")


def extract_code_block(response: str, lang: str = "c") -> str:
    """Extract the first ```<lang> ... ``` fenced code block from a response."""
    pattern = rf"```{re.escape(lang)}\s*\n(.*?)```"
    match = re.search(pattern, response, re.DOTALL)
    if match:
        return match.group(1).strip() + "\n"
    # Fallback: try any fenced block
    match = re.search(r"```\w*\s*\n(.*?)```", response, re.DOTALL)
    if match:
        return match.group(1).strip() + "\n"
    return response.strip() + "\n"


def extract_search_replace_blocks(response: str) -> list[tuple[str, str]]:
    """Parse all <<<SEARCH ... === ... >>> blocks from an LLM response.

    Returns a list of (search, replace) string tuples.
    """
    pattern = r"<<<SEARCH\n(.*?)\n===\n(.*?)\n>>>"
    return re.findall(pattern, response, re.DOTALL)


def apply_search_replace(source: str, blocks: list[tuple[str, str]]) -> str:
    """Apply search-replace blocks sequentially to source code.

    Matching strategy: exact match first, then retry with leading/trailing
    blank lines stripped from the search string. Raises ValueError on failure.
    """
    for i, (search, replace) in enumerate(blocks):
        if search in source:
            source = source.replace(search, replace, 1)
            continue
        # Retry: strip leading/trailing blank lines from search
        stripped = search.strip("\n")
        if stripped and stripped in source:
            source = source.replace(stripped, replace.strip("\n"), 1)
            continue
        # Show a short preview of the failing search block
        preview = search[:120].replace("\n", "\\n")
        raise ValueError(
            f"Block {i+1}/{len(blocks)} failed to match: '{preview}...'"
        )
    return source


def image_to_base64(path: Path) -> str:
    """Read an image file and return its base64-encoded string."""
    data = path.read_bytes()
    return base64.b64encode(data).decode("ascii")


if __name__ == "__main__":
    # Self-test for search-replace functions
    resp = "Some text\n<<<SEARCH\nint x = 1;\n===\nint x = 2;\n>>>\nmore\n<<<SEARCH\nfoo();\n===\nbar();\n>>>"
    blocks = extract_search_replace_blocks(resp)
    assert blocks == [("int x = 1;", "int x = 2;"), ("foo();", "bar();")], f"extract failed: {blocks}"

    src = "int x = 1;\nfoo();\n"
    result = apply_search_replace(src, blocks)
    assert result == "int x = 2;\nbar();\n", f"apply failed: {result!r}"

    assert extract_search_replace_blocks("no blocks here") == []

    try:
        apply_search_replace("nothing", [("missing", "x")])
        assert False, "should have raised"
    except ValueError:
        pass

    print("llm_client self-tests passed")
