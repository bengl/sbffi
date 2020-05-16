const path = require('path');
const fs = require('fs');
const execSync = require('child_process');

if (!fs.existsSync(path.join(__dirname, 'adder', 'libadder.so'))) {
  execSync('make', { cwd: path.join(__dirname, 'adder') });
}

if (!fs.existsSync(path.join(__dirname, 'napiaddon', 'build', 'Release', 'napi.node'))) {
  execSync('npm install', { cwd: path.join(__dirname, 'napiaddon') });
}

