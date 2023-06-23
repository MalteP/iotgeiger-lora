// Payload decoder for TTN v3 & Chirpstack v4, JS code based on ATTNode v3 example (https://www.attno.de/)

function bytesToInt16(bytes, start) {
  var out = ((bytes[start]) | (bytes[start + 1] << 8));
  var sign = bytes[start + 1] & (1 << 7);
  if (sign) {
    out = 0xFFFF0000 | out;
  }
  return out;
}

function bytesToUInt16(bytes, start) {
  return ((bytes[start]) | (bytes[start + 1] << 8));
}

function bytesToInt32(bytes, start) {
  return ((bytes[start]) | (bytes[start + 1] << 8) | (bytes[start + 2] << 16) | (bytes[start + 3] << 24));
}

function decodeUplink(input) {
  var decoded = {};
  var i = 0;

  if (input.bytes.length >= 2) {
    // CPM (uint16_t)
    decoded.cpm = bytesToUInt16(input.bytes, i);
    decoded.usv_h = Math.round((decoded.cpm * 0.00570027) * 1000) / 1000;
    i += 2;
  }

  if (input.bytes.length >= 14) {
    // Temperature (int32_t)
    decoded.temperature = bytesToInt32(input.bytes, i) / 100;
    i += 4;

    // Atmospheric Pressure (int32_t)
    decoded.pressure = bytesToInt32(input.bytes, i) / 100;
    i += 4;

    // Humidity (int32_t)
    decoded.humidity = bytesToInt32(input.bytes, i) / 1000;
    i += 4;
  }

  return {
    data: decoded,
    warnings: [],
    errors: []
  };
}