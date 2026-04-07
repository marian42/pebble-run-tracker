const USE_DEBUG_POSITION = false;

var locationWatchId = null;

Pebble.addEventListener("ready",
  function (e) {
    if (USE_DEBUG_POSITION) {
      setInterval(sendSyntheticPosition, INTERVAL_SEC * 1000);
    } else {
      locationWatchId = navigator.geolocation.watchPosition(
        function (pos) {
          sendPosition(pos.coords.latitude, pos.coords.longitude, pos.coords.accuracy);
        },
        function (err) {
          console.log("GPS error: " + err.message);
        },
        { enableHighAccuracy: true, maximumAge: 0, timeout: 5000 }
      );
    }
  }
);


function sendPosition(latitude, longitude, accuracy) {
  Pebble.sendAppMessage({
    "latitude": Math.round(latitude * 1e6),
    "longitude": Math.round(longitude * 1e6),
    "accuracy": Math.round(accuracy * 10),
  });
}

// Simulate movement for debugging

var CENTER_LAT = 51.4790275;
var CENTER_LON = -0.1568381;
var EARTH_RADIUS = 6371000;
var CIRCUMFERENCE = 1000; // Move around a 1000m circle
var RADIUS = CIRCUMFERENCE / (2 * Math.PI);
var SPEED_MPS = 8000 / 3600; // 8 km/h in m/s
var INTERVAL_SEC = 2;
var STEP_DISTANCE = SPEED_MPS * INTERVAL_SEC;
var ANGLE_STEP = STEP_DISTANCE / RADIUS;
var NOISE_METERS = 3;

var currentAngle = 0;

function sendSyntheticPosition() {
  currentAngle += ANGLE_STEP;
  if (currentAngle > 2 * Math.PI) {
    currentAngle -= 2 * Math.PI;
  }

  let x = RADIUS * Math.cos(currentAngle) + (Math.random() * 2 - 1) * NOISE_METERS;
  let y = RADIUS * Math.sin(currentAngle) + (Math.random() * 2 - 1) * NOISE_METERS;

  let lat = CENTER_LAT + y / 111320;
  let lon = CENTER_LON + x / (111320 * Math.cos(lat * Math.PI / 180));
  
  sendPosition(lat, lon, 0);
}
