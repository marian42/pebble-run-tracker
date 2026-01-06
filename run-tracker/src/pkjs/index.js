Pebble.addEventListener("ready",
    function(e) {
        console.log("Hello world! - Sent from your javascript application.");
        let timer = setInterval(sendSyntheticPosition, INTERVAL_SEC * 1000);
    }
);


function sendPosition(latitude, longitude) {
    Pebble.sendAppMessage({
      "latitude": Math.round(latitude * 1e6),
      "longitude": Math.round(longitude * 1e6),
    });
}

var watchId = navigator.geolocation.watchPosition(
  function(pos) {
    console.log("got a GPS position! " + pos.coords.latitude + ", " + pos.coords.longitude);
    sendPosition(pos.coords.latitude, pos.coords.longitude);
  },
  function(err) {
    console.log("GPS error: " + err.message);
  },
  { enableHighAccuracy: true, maximumAge: 0, timeout: 5000 }
);


// Clear the watch and stop receiving updates
// navigator.geolocation.clearWatch(watchId);

// ---- Pebble GPS simulator ----

// Pebble message keys (must match C enum)
var KEY_LAT = 0;
var KEY_LON = 1;

// Center point (degrees)
var CENTER_LAT = 51.4790275;
var CENTER_LON = -0.1568381;

// Earth radius (meters)
var EARTH_RADIUS = 6371000;

// Circle parameters
var CIRCUMFERENCE = 1000; // meters
var RADIUS = CIRCUMFERENCE / (2 * Math.PI); // ≈159.155 m

// Speed
var SPEED_MPS = 8000 / 3600; // 8 km/h → m/s
var INTERVAL_SEC = 2;
var STEP_DISTANCE = SPEED_MPS * INTERVAL_SEC;

// Angular velocity (radians per step)
var ANGLE_STEP = STEP_DISTANCE / RADIUS;

// Noise (meters)
var NOISE_METERS = 3;

// State
var angle = 0;
var timer = null;

// Convert meters to latitude degrees
function metersToLat(m) {
  return m / 111320;
}

// Convert meters to longitude degrees (depends on latitude)
function metersToLon(m, latDeg) {
  return m / (111320 * Math.cos(latDeg * Math.PI / 180));
}

// Generate next position
function getSyntheticPosition() {
  angle += ANGLE_STEP;
  if (angle > 2 * Math.PI) {
    angle -= 2 * Math.PI;
  }

  // Ideal circle position (meters)
  var x = RADIUS * Math.cos(angle);
  var y = RADIUS * Math.sin(angle);

  // Add random noise
  x += (Math.random() * 2 - 1) * NOISE_METERS;
  y += (Math.random() * 2 - 1) * NOISE_METERS;

  // Convert to degrees
  var lat = CENTER_LAT + metersToLat(y);
  var lon = CENTER_LON + metersToLon(x, CENTER_LAT);

  return { lat, lon };
}

// Send to Pebble
function sendSyntheticPosition() {
  var pos = getSyntheticPosition();
  sendPosition(pos.lat, pos.lon);
}
