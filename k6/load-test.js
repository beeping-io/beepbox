import http from "k6/http";
import { check, sleep } from "k6";

const BASE_URL = __ENV.BASE_URL || "https://beepbox-server-ai7n45q5lq-ew.a.run.app";

export const options = {
  scenarios: {
    ramp_up: {
      executor: "ramping-vus",
      startVUs: 0,
      stages: [{ duration: "30s", target: 50 }],
      gracefulStop: "5s",
    },
    sustained: {
      executor: "constant-vus",
      vus: 50,
      duration: "2m",
      startTime: "30s",
      gracefulStop: "5s",
    },
    spike: {
      executor: "ramping-vus",
      startVUs: 50,
      stages: [
        { duration: "10s", target: 200 },
        { duration: "20s", target: 200 },
        { duration: "10s", target: 50 },
      ],
      startTime: "2m30s",
      gracefulStop: "5s",
    },
  },
  thresholds: {
    http_req_duration: ["p(95)<500"],
    http_req_failed: ["rate<0.01"],
  },
};

const ENCODE_PAYLOAD = JSON.stringify({
  key: "a1b2c",
  mode: "inaudible",
  duration: 2.3,
});

export default function () {
  // Encode request
  const encRes = http.post(`${BASE_URL}/v1/encode`, ENCODE_PAYLOAD, {
    headers: { "Content-Type": "application/json" },
  });

  check(encRes, {
    "encode status 200": (r) => r.status === 200,
    "encode returns WAV": (r) => r.body && r.body.length > 44,
  });

  // Version check (lightweight — mix in some GET requests)
  if (Math.random() < 0.1) {
    const verRes = http.get(`${BASE_URL}/version`);
    check(verRes, {
      "version status 200": (r) => r.status === 200,
    });
  }

  sleep(0.1);
}
