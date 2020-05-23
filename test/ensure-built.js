const path = require('path');
const fs = require('fs');
const { execSync } = require('child_process');
const os = require('os');

const make = os.type() === 'Windows_NT' ? 'mingw32-make' : 'make';

if (!fs.existsSync(path.join(__dirname, 'adder', 'libadder.so'))) {
  execSync(make, { cwd: path.join(__dirname, 'adder') });
}

if (!fs.existsSync(path.join(__dirname, 'napiaddon', 'build', 'Release', 'napi.node'))) {
  execSync('npm install', { cwd: path.join(__dirname, 'napiaddon') });
}
