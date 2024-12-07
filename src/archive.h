// TEA Format:
//
// signature: 3 bytes
// padding: 1 byte
// data: n bytes
// padding: 1 byte
// data headers: n bytes
// padding: 1 byte
// common flags: n bytes
// padding: 1 byte
// metadata: n bytes
// padding: 1 byte
// header: 42 bytes
// padding: 1 byte
// signature: 3 bytes
//
// header:
//
// number of files: 8 bytes
// position of data headers: 8 bytes
// number of unique flags: 2 bytes
// position of common flags: 8 bytes
// size of metadata: 2 bytes
// position of metadata: 8 bytes
// global flags: 2 bytes
// reserved: 4 bytes
//
// file header:
//
// size of name: 2 bytes
// name: n bytes
// size of data: 8 bytes
// position of parent: 8 bytes (0 if no parent)
// offset from data: 8 bytes
// epoch modification time: 8 bytes
// reserved: 1 byte
//
// Flags (archive header):
//
// b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16
//
// b1: encrypted
// b2: compressed
// b3: compression type
// b4: compression type
// b5: compression strength
// b6: compression strength
// >= b7: reserved
//
// Flags (common flags):
//
// b1, b2, b3, b4, b5, b6, b7, b8, ...
//
// b1: encrypted
// b2: compressed
// b3: compression type
// b4: compression type
// b5: directory
// >= b6: reserved
//
// Data header:
//
// end of file headers: 8 bytes
// file headers: n bytes
// positions of file headers: n * 8 bytes
