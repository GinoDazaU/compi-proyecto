#!/usr/bin/env python3
"""Quick test for the backend API."""
import sys
sys.path.insert(0, ".")
from main import app
from fastapi.testclient import TestClient

client = TestClient(app)

# Test health
r = client.get("/api/health")
print("Health:", r.json())

# Test compile
r = client.post("/api/compile", json={"code": "int main() { return 0; }"})
d = r.json()
print("Compile success:", d["success"])
print("Tokens count:", len(d.get("tokens", [])))
print("AST type:", d.get("ast", {}).get("type"))

# Test error
r = client.post("/api/compile", json={"code": "int main() { @@ }"})
d = r.json()
print("Error test success:", not d["success"])
print("Error type:", d.get("error", {}).get("type"))
