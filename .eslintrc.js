module.exports = {
  env: {
    node: true,
    commonjs: true,
    es6: true
  },
  extends: [
    'standard'
  ],
  parserOptions: {
    ecmaVersion: 11
  },
  rules: {
    semi: ['error', 'always']
  }
};
