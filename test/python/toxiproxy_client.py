import json
from urllib import error, request

TOXIPROXY_API_HOST = "127.0.0.1"
TOXIPROXY_API_PORT = "8474"
TOXIPROXY_API_URL = f"http://{TOXIPROXY_API_HOST}:{TOXIPROXY_API_PORT}"


class ToxiproxyClient:
    """Client for Toxiproxy (https://github.com/Shopify/toxiproxy)

    Used to simulate faulty network conditions (for example simulating a disconnection)
    """

    @staticmethod
    def _request(method: str, path: str, body: dict | None = None):
        data = None
        headers = {}

        if body is not None:
            data = json.dumps(body).encode("utf-8")
            headers["Content-Type"] = "application/json"

        req = request.Request(
            f"{TOXIPROXY_API_URL}{path}",
            data=data,
            headers=headers,
            method=method,
        )

        try:
            return request.urlopen(req, timeout=5)
        except error.HTTPError as e:
            return e

    def create_proxy(self, name: str, listen: str, upstream: str) -> None:
        # Clean up any stale proxy left over from a previous run.
        self._request("DELETE", f"/proxies/{name}")

        r = self._request(
            "POST",
            "/proxies",
            {
                "name": name,
                "listen": listen,
                "upstream": upstream,
                "enabled": True,
            },
        )

        if r.status >= 400:
            raise RuntimeError(f"Toxiproxy returned HTTP {r.status}")

    def delete_proxy(self, name: str) -> None:
        r = self._request("DELETE", f"/proxies/{name}")

        if r.status not in (200, 204, 404):
            raise RuntimeError(f"Toxiproxy returned HTTP {r.status}")

    def cut_connection(self, name: str) -> None:
        """Simulates a connection cut through the proxy in both directions"""
        for stream in ("upstream", "downstream"):
            r = self._request(
                "POST",
                f"/proxies/{name}/toxics",
                {
                    "name": f"cut_{stream}",
                    "type": "timeout",
                    "stream": stream,
                    "toxicity": 1.0,  # Probability of the toxic being applied
                    "attributes": {"timeout": 0},  # Disconnection simulation
                },
            )

            if r.status >= 400:
                raise RuntimeError(f"Toxiproxy returned HTTP {r.status}")

    def restore_connection(self, name: str) -> None:
        for stream in ("upstream", "downstream"):
            r = self._request(
                "DELETE",
                f"/proxies/{name}/toxics/cut_{stream}",
            )

            if r.status not in (200, 204, 404):
                raise RuntimeError(f"Toxiproxy returned HTTP {r.status}")
