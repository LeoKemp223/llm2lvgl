#!/usr/bin/env python3
"""CLI entry point for llm2lvgl Web UI."""

import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="llm2lvgl Web UI")
    parser.add_argument("--host", default="127.0.0.1", help="bind address (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=5000, help="port (default: 5000)")
    parser.add_argument("--debug", action="store_true", help="enable Flask debug mode")
    args = parser.parse_args()

    try:
        from web.server import create_app
    except ImportError:
        # When invoked from outside tools/
        sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parent))
        from web.server import create_app

    app = create_app()
    print(f"llm2lvgl Web UI: http://{args.host}:{args.port}")
    app.run(host=args.host, port=args.port, debug=args.debug)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
