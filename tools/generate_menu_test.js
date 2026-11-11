// Builds menu_test/sketch.ino from the menu JSON (tools/menu-editor/menu-v2.json), the generated tables and the menu engine.
//   node tools/generate_menu_test.js
'use strict';
const fs = require('fs'), path = require('path');
const root = path.join(__dirname, '..');
const { tables } = require(path.join(__dirname, 'menu_engine', 'generate_menu_tables.js'));
const menu = JSON.parse(fs.readFileSync(path.join(root, 'tools', 'menu-editor', 'menu-v2.json'), 'utf8'));
let shell = fs.readFileSync(path.join(__dirname, 'menu_engine', 'test_shell.ino.in'), 'utf8');
const engine = fs.readFileSync(path.join(__dirname, 'menu_engine', 'engine.inc'), 'utf8');
shell = shell.replace('//@TABLES@\n', () => tables(menu, 'menu-v2.json')).replace('//@ENGINE@\n', () => engine);
fs.mkdirSync(path.join(root, 'menu_test'), { recursive: true });
fs.writeFileSync(path.join(root, 'menu_test', 'sketch.ino'), shell);
console.log('Wrote menu_test/sketch.ino (' + shell.length + ' bytes)');
