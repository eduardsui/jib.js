var path = require('path');


console.log(path.join('/foo', 'bar', 'baz/asdf', 'quux', '..'));
console.log(path.resolve("src", "misc"));
console.log(path.extname('index.html'));
console.log(path.extname('index'));
console.log(path.extname('.index'));
console.log(path.dirname('/foo/bar/baz/asdf/quux'));
console.log(path.basename('/foo/bar/baz/asdf/quux.html'));
console.log(path.basename('/foo/bar/baz/asdf/quux.html', '.html'));

console.log(path.format({
  root: '/ignored',
  dir: '/home/user/dir',
  base: 'file.txt'
}));

console.log(path.format({
  root: '/',
  base: 'file.txt',
  ext: 'ignored'
}));

console.log(path.format({
  root: '/',
  name: 'file',
  ext: '.txt'
}));

console.log(path.parse('C:/home/user/dir/file.txt'));
console.log(path.relative('/data/orandea/test/aaa', '/data/orandea/impl/bbb'));