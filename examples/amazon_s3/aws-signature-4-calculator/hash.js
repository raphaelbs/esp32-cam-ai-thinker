const crypto = require('crypto');

function hmacSHA256(signKey, text) {
  return crypto.createHmac('sha256', signKey).update(text).digest();
}

function hmacSHA256Hex(signKey, text) {
  return crypto.createHmac('sha256', signKey).update(text).digest('hex');
}

exports.hmacSHA256 = hmacSHA256;
exports.hmacSHA256Hex = hmacSHA256Hex;
