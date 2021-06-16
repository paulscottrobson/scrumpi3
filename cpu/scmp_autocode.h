static BYTE8 a,carry,overflow,e,s,temp8;
static WORD16 p0,p1,p2,p3,temp16;
static BYTE8 constructStatus(void)  {
  return (s & 0xF) | ((readSenseLines() & 0x3) << 4) | (overflow << 6) | (carry << 7);
}
static void resetProcessor() {
 a = e = overflow = carry = s = 0;
 p0 = p1 = p2 = p3 = 0;
 branch(p0);writeIOFlags(0);
}
static BYTE8 decimalAdd(BYTE8 d1,BYTE8 d2) {
 BYTE8 lsd,msd;
 lsd = (d1 & 0xF) + (d2 & 0xF) + carry;
 msd = ((d1 >> 4) & 0xF) + ((d2 >> 4) & 0xF);
  carry = 0;
  if (lsd >= 10) { msd++;lsd = lsd - 10; }
 if (msd >= 10) { carry = 1;msd = msd - 10; }
 return lsd + (msd << 4);
}
static BYTE8 binaryAdd(BYTE8 d1,BYTE8 d2) {
 WORD16 result = d1 + d2 + ((carry != 0) ? 1 : 0);
 carry = (result >= 0x100) ? 1 : 0;
 overflow = 0;
 if ((d1 & 0x80) == (d2 & 0x80) && (d1 & 0x80) != (result & 0x80)) overflow = 1;
 return (BYTE8)(result & 0xFF);
}
static WORD16 eacRelative(BYTE8 offset,WORD16 pointer) {
 if (offset == 0x80) offset = e;
 if (offset & 0x80)
  return addAddress(pointer,offset - 0x100);
  return addAddress(pointer,offset);
}

static WORD16 eacAutoIndex(BYTE8 offset,WORD16 *pPointer) {
 if (offset == 0x80) offset = e;
 if (offset & 0x80) {
  *pPointer = addAddress(*pPointer,offset - 0x100);
  return *pPointer;
 } else {
  WORD16 address = *pPointer;
  *pPointer = addAddress(*pPointer,offset);
  return address;
 }
}
