case 0x00: /* halt */
    addCycles(8);{};break;
case 0x01: /* xae */
    addCycles(7);temp8 = a;a = e;e = temp8;break;
case 0x02: /* ccl */
    addCycles(5);carry = 0;break;
case 0x03: /* scl */
    addCycles(5);carry = 1;break;
case 0x04: /* dint */
    addCycles(6);s = s & 0xF7;break;
case 0x05: /* ien */
    addCycles(6);s = s | 0x08;break;
case 0x06: /* csa */
    addCycles(5);a = constructStatus();break;
case 0x07: /* cas */
    addCycles(6);s = a;writeIOFlags(s & 7);overflow = (s & 0x40) >> 6;carry = (s & 0x80) >> 7;;break;
case 0x08: /* nop */
    addCycles(5);{};break;
case 0x19: /* sio */
    addCycles(5);e = (e >> 1) & 0x7F;break;
case 0x1c: /* sr */
    addCycles(5);a = (a >> 1) & 0x7F;break;
case 0x1d: /* srl */
    addCycles(5);a = a >> 1;if (carry != 0) a = a | 0x80;break;
case 0x1e: /* rr */
    addCycles(5);a = (a >> 1) | ((a & 1) << 7);break;
case 0x1f: /* rrl */
    addCycles(5);temp16 = a;if (carry != 0) temp16 |= 0x80;a = (temp16 >> 1) & 0xFF;carry = temp16 & 1;break;
case 0x30: /* xpal 0 */
    addCycles(8);temp8 = p0 & 0xFF;p0 = (p0 & 0xFF00) | a;a = temp8; branch(p0);;break;
case 0x31: /* xpal 1 */
    addCycles(8);temp8 = p1 & 0xFF;p1 = (p1 & 0xFF00) | a;a = temp8;;break;
case 0x32: /* xpal 2 */
    addCycles(8);temp8 = p2 & 0xFF;p2 = (p2 & 0xFF00) | a;a = temp8;;break;
case 0x33: /* xpal 3 */
    addCycles(8);temp8 = p3 & 0xFF;p3 = (p3 & 0xFF00) | a;a = temp8;;break;
case 0x34: /* xpah 0 */
    addCycles(8);temp8 = (p0 >> 8) & 0xFF;p0 = (p0 & 0xFF) | (((WORD16)a) << 8);a = temp8; branch(p0);;break;
case 0x35: /* xpah 1 */
    addCycles(8);temp8 = (p1 >> 8) & 0xFF;p1 = (p1 & 0xFF) | (((WORD16)a) << 8);a = temp8;;break;
case 0x36: /* xpah 2 */
    addCycles(8);temp8 = (p2 >> 8) & 0xFF;p2 = (p2 & 0xFF) | (((WORD16)a) << 8);a = temp8;;break;
case 0x37: /* xpah 3 */
    addCycles(8);temp8 = (p3 >> 8) & 0xFF;p3 = (p3 & 0xFF) | (((WORD16)a) << 8);a = temp8;;break;
case 0x3c: /* xppc 0 */
    addCycles(7);{};break;
case 0x3d: /* xppc 1 */
    addCycles(7);temp16 = p1;p1 = p0;p0 = temp16; branch(p0);break;
case 0x3e: /* xppc 2 */
    addCycles(7);temp16 = p2;p2 = p0;p0 = temp16; branch(p0);break;
case 0x3f: /* xppc 3 */
    addCycles(7);temp16 = p3;p3 = p0;p0 = temp16; branch(p0);break;
case 0x40: /* lde */
    addCycles(6);a = e;break;
case 0x50: /* ane */
    addCycles(6);a = a & e;break;
case 0x58: /* ore */
    addCycles(6);a = a | e;break;
case 0x60: /* xre */
    addCycles(6);a = a ^ e;break;
case 0x68: /* dae */
    addCycles(11);a = decimalAdd(a,e);break;
case 0x70: /* ade */
    addCycles(7);a = binaryAdd(a,e);break;
case 0x78: /* cae */
    addCycles(8);a = binaryAdd(a,e ^ 0xFF);break;
case 0x8f: /* dly @1 */
    addCycles(0);longDelay(operand,a);a = 0;break;
case 0x90: /* jmp @1(p) */
    addCycles(11);branch(eacRelative(operand,p0));break;
case 0x91: /* jmp @1(1) */
    addCycles(11);branch(eacRelative(operand,p1));break;
case 0x92: /* jmp @1(2) */
    addCycles(11);branch(eacRelative(operand,p2));break;
case 0x93: /* jmp @1(3) */
    addCycles(11);branch(eacRelative(operand,p3));break;
case 0x94: /* jp @1(p) */
    addCycles(10);if ((a & 0x80) == 0) branch(eacRelative(operand,p0));break;
case 0x95: /* jp @1(1) */
    addCycles(10);if ((a & 0x80) == 0) branch(eacRelative(operand,p1));break;
case 0x96: /* jp @1(2) */
    addCycles(10);if ((a & 0x80) == 0) branch(eacRelative(operand,p2));break;
case 0x97: /* jp @1(3) */
    addCycles(10);if ((a & 0x80) == 0) branch(eacRelative(operand,p3));break;
case 0x98: /* jz @1(p) */
    addCycles(10);if (a == 0)  branch(eacRelative(operand,p0));break;
case 0x99: /* jz @1(1) */
    addCycles(10);if (a == 0)  branch(eacRelative(operand,p1));break;
case 0x9a: /* jz @1(2) */
    addCycles(10);if (a == 0)  branch(eacRelative(operand,p2));break;
case 0x9b: /* jz @1(3) */
    addCycles(10);if (a == 0)  branch(eacRelative(operand,p3));break;
case 0x9c: /* jnz @1(p) */
    addCycles(10);if (a != 0)  branch(eacRelative(operand,p0));break;
case 0x9d: /* jnz @1(1) */
    addCycles(10);if (a != 0)  branch(eacRelative(operand,p1));break;
case 0x9e: /* jnz @1(2) */
    addCycles(10);if (a != 0)  branch(eacRelative(operand,p2));break;
case 0x9f: /* jnz @1(3) */
    addCycles(10);if (a != 0)  branch(eacRelative(operand,p3));break;
case 0xa8: /* ild @1(p) */
    addCycles(22);temp16 = eacRelative(operand,p0);a = (readByte(temp16)+1) & 0xFF;writeByte(temp16,a);break;
case 0xa9: /* ild @1(1) */
    addCycles(22);temp16 = eacRelative(operand,p1);a = (readByte(temp16)+1) & 0xFF;writeByte(temp16,a);break;
case 0xaa: /* ild @1(2) */
    addCycles(22);temp16 = eacRelative(operand,p2);a = (readByte(temp16)+1) & 0xFF;writeByte(temp16,a);break;
case 0xab: /* ild @1(3) */
    addCycles(22);temp16 = eacRelative(operand,p3);a = (readByte(temp16)+1) & 0xFF;writeByte(temp16,a);break;
case 0xb8: /* dld @1(p) */
    addCycles(22);temp16 = eacRelative(operand,p0);a = (readByte(temp16)-1) & 0xFF;writeByte(temp16,a);break;
case 0xb9: /* dld @1(1) */
    addCycles(22);temp16 = eacRelative(operand,p1);a = (readByte(temp16)-1) & 0xFF;writeByte(temp16,a);break;
case 0xba: /* dld @1(2) */
    addCycles(22);temp16 = eacRelative(operand,p2);a = (readByte(temp16)-1) & 0xFF;writeByte(temp16,a);break;
case 0xbb: /* dld @1(3) */
    addCycles(22);temp16 = eacRelative(operand,p3);a = (readByte(temp16)-1) & 0xFF;writeByte(temp16,a);break;
case 0xc0: /* ld @1(p) */
    addCycles(18);a = readByte(eacRelative(operand,p0));break;
case 0xc1: /* ld @1(1) */
    addCycles(18);a = readByte(eacRelative(operand,p1));break;
case 0xc2: /* ld @1(2) */
    addCycles(18);a = readByte(eacRelative(operand,p2));break;
case 0xc3: /* ld @1(3) */
    addCycles(18);a = readByte(eacRelative(operand,p3));break;
case 0xc4: /* ldi @1 */
    addCycles(10);a = operand;break;
case 0xc5: /* ld |@1(1) */
    addCycles(18);a = readByte(eacAutoIndex(operand,&p1));break;
case 0xc6: /* ld |@1(2) */
    addCycles(18);a = readByte(eacAutoIndex(operand,&p2));break;
case 0xc7: /* ld |@1(3) */
    addCycles(18);a = readByte(eacAutoIndex(operand,&p3));break;
case 0xc8: /* st @1(p) */
    addCycles(18);writeByte(eacRelative(operand,p0),a);break;
case 0xc9: /* st @1(1) */
    addCycles(18);writeByte(eacRelative(operand,p1),a);break;
case 0xca: /* st @1(2) */
    addCycles(18);writeByte(eacRelative(operand,p2),a);break;
case 0xcb: /* st @1(3) */
    addCycles(18);writeByte(eacRelative(operand,p3),a);break;
case 0xcd: /* st |@1(1) */
    addCycles(18);writeByte(eacAutoIndex(operand,&p1),a);break;
case 0xce: /* st |@1(2) */
    addCycles(18);writeByte(eacAutoIndex(operand,&p2),a);break;
case 0xcf: /* st |@1(3) */
    addCycles(18);writeByte(eacAutoIndex(operand,&p3),a);break;
case 0xd0: /* and @1(p) */
    addCycles(18);a = a & readByte(eacRelative(operand,p0));break;
case 0xd1: /* and @1(1) */
    addCycles(18);a = a & readByte(eacRelative(operand,p1));break;
case 0xd2: /* and @1(2) */
    addCycles(18);a = a & readByte(eacRelative(operand,p2));break;
case 0xd3: /* and @1(3) */
    addCycles(18);a = a & readByte(eacRelative(operand,p3));break;
case 0xd4: /* ani @1 */
    addCycles(10);a = a & operand;break;
case 0xd5: /* and |@1(1) */
    addCycles(18);a = a & readByte(eacAutoIndex(operand,&p1));break;
case 0xd6: /* and |@1(2) */
    addCycles(18);a = a & readByte(eacAutoIndex(operand,&p2));break;
case 0xd7: /* and |@1(3) */
    addCycles(18);a = a & readByte(eacAutoIndex(operand,&p3));break;
case 0xd8: /* or @1(p) */
    addCycles(18);a = a | readByte(eacRelative(operand,p0));break;
case 0xd9: /* or @1(1) */
    addCycles(18);a = a | readByte(eacRelative(operand,p1));break;
case 0xda: /* or @1(2) */
    addCycles(18);a = a | readByte(eacRelative(operand,p2));break;
case 0xdb: /* or @1(3) */
    addCycles(18);a = a | readByte(eacRelative(operand,p3));break;
case 0xdc: /* ori @1 */
    addCycles(10);a = a | operand;break;
case 0xdd: /* or |@1(1) */
    addCycles(18);a = a | readByte(eacAutoIndex(operand,&p1));break;
case 0xde: /* or |@1(2) */
    addCycles(18);a = a | readByte(eacAutoIndex(operand,&p2));break;
case 0xdf: /* or |@1(3) */
    addCycles(18);a = a | readByte(eacAutoIndex(operand,&p3));break;
case 0xe0: /* xor @1(p) */
    addCycles(18);a = a ^ readByte(eacRelative(operand,p0));break;
case 0xe1: /* xor @1(1) */
    addCycles(18);a = a ^ readByte(eacRelative(operand,p1));break;
case 0xe2: /* xor @1(2) */
    addCycles(18);a = a ^ readByte(eacRelative(operand,p2));break;
case 0xe3: /* xor @1(3) */
    addCycles(18);a = a ^ readByte(eacRelative(operand,p3));break;
case 0xe4: /* xri @1 */
    addCycles(10);a = a ^ operand;break;
case 0xe5: /* xor |@1(1) */
    addCycles(18);a = a ^ readByte(eacAutoIndex(operand,&p1));break;
case 0xe6: /* xor |@1(2) */
    addCycles(18);a = a ^ readByte(eacAutoIndex(operand,&p2));break;
case 0xe7: /* xor |@1(3) */
    addCycles(18);a = a ^ readByte(eacAutoIndex(operand,&p3));break;
case 0xe8: /* dad @1(p) */
    addCycles(23);a = decimalAdd(a,readByte(eacRelative(operand,p0)));break;
case 0xe9: /* dad @1(1) */
    addCycles(23);a = decimalAdd(a,readByte(eacRelative(operand,p1)));break;
case 0xea: /* dad @1(2) */
    addCycles(23);a = decimalAdd(a,readByte(eacRelative(operand,p2)));break;
case 0xeb: /* dad @1(3) */
    addCycles(23);a = decimalAdd(a,readByte(eacRelative(operand,p3)));break;
case 0xec: /* dai @1 */
    addCycles(15);a = decimalAdd(a,operand);break;
case 0xed: /* dad |@1(1) */
    addCycles(23);a = decimalAdd(a,readByte(eacAutoIndex(operand,&p1)));break;
case 0xee: /* dad |@1(2) */
    addCycles(23);a = decimalAdd(a,readByte(eacAutoIndex(operand,&p2)));break;
case 0xef: /* dad |@1(3) */
    addCycles(23);a = decimalAdd(a,readByte(eacAutoIndex(operand,&p3)));break;
case 0xf0: /* add @1(p) */
    addCycles(19);a = binaryAdd(a,readByte(eacRelative(operand,p0)));break;
case 0xf1: /* add @1(1) */
    addCycles(19);a = binaryAdd(a,readByte(eacRelative(operand,p1)));break;
case 0xf2: /* add @1(2) */
    addCycles(19);a = binaryAdd(a,readByte(eacRelative(operand,p2)));break;
case 0xf3: /* add @1(3) */
    addCycles(19);a = binaryAdd(a,readByte(eacRelative(operand,p3)));break;
case 0xf4: /* adi @1 */
    addCycles(11);a = binaryAdd(a,operand);break;
case 0xf5: /* add |@1(1) */
    addCycles(19);a = binaryAdd(a,readByte(eacAutoIndex(operand,&p1)));break;
case 0xf6: /* add |@1(2) */
    addCycles(19);a = binaryAdd(a,readByte(eacAutoIndex(operand,&p2)));break;
case 0xf7: /* add |@1(3) */
    addCycles(19);a = binaryAdd(a,readByte(eacAutoIndex(operand,&p3)));break;
case 0xf8: /* cad @1(p) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacRelative(operand,p0)));break;
case 0xf9: /* cad @1(1) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacRelative(operand,p1)));break;
case 0xfa: /* cad @1(2) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacRelative(operand,p2)));break;
case 0xfb: /* cad @1(3) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacRelative(operand,p3)));break;
case 0xfc: /* cai @1 */
    addCycles(12);a = binaryAdd(a,0xFF ^ operand);break;
case 0xfd: /* cad |@1(1) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacAutoIndex(operand,&p1)));break;
case 0xfe: /* cad |@1(2) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacAutoIndex(operand,&p2)));break;
case 0xff: /* cad |@1(3) */
    addCycles(20);a = binaryAdd(a,0xFF ^ readByte(eacAutoIndex(operand,&p3)));break;
