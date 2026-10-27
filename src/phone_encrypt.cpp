// Ported from ZhihuMessageBodyEncryptor.kt, Copyright (C) 2024-2026 zly2006.
// AGPL-3.0-only; protocol tables and byte transforms are retained verbatim.
#include "phone_login.hpp"
static const QByteArray &protocolData() {
    static const QByteArray data = QByteArray::fromBase64(
"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"
    ); return data;
}
static unsigned char swapPairs(unsigned char v) { return ((v & 0x55)<<1) | ((v & 0xaa)>>1); }
static unsigned char tableXor(const unsigned char *table, unsigned char a, unsigned char b) {
    return (table[((a>>4)<<4) ^ (b>>4)] & 0xf0) ^ (table[((a & 15)<<4) ^ (b & 15)]>>4);
}
static void encryptBlock(const unsigned char *block, unsigned char *result) {
    const unsigned char *data = reinterpret_cast<const unsigned char *>(protocolData().constData());
    unsigned char state[16], mixed[16];
    for(int i=0;i<16;++i) state[i]=tableXor(data+176, block[i], data[i]);
    const int positions[4][4]={{0,4,8,12},{5,9,13,1},{10,14,2,6},{15,3,7,11}};
    for(int round=1;round<10;++round) {
        for(int t=0;t<4;++t) for(int word=0;word<4;++word) for(int byte=0;byte<4;++byte) {
            const int index=word*4+byte;
            const unsigned char value=data[944+t*1024+state[positions[t][word]]*4+3-byte];
            mixed[index]=t==0 ? value : tableXor(data+432,mixed[index],value);
        }
        for(int i=0;i<16;++i) state[i]=tableXor(data+432,mixed[i],data[round*16+i]);
    }
    const int shifted[16]={0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11};
    for(int i=0;i<16;++i) result[i]=tableXor(data+688,data[5040+state[shifted[i]]],data[160+i]);
}
QByteArray PhoneLogin::encrypt(const QByteArray &plain) {
    const int padding=16-plain.size()%16;
    QByteArray input(plain.size()+padding, char(swapPairs(padding)^0xbb));
    for(int i=0;i<plain.size();++i) input[i]=char(swapPairs(static_cast<unsigned char>(plain.at(i)))^0xbb);
    unsigned char previous[16], block[16]; const char *iv="f0551856aa575faa";
    for(int i=0;i<16;++i) previous[i]=swapPairs(iv[i]);
    QByteArray output(input.size(), '\0');
    for(int offset=0;offset<input.size();offset+=16) {
        for(int i=0;i<16;++i) block[i]=static_cast<unsigned char>(input.at(offset+i))^previous[i];
        encryptBlock(block,previous);
        for(int i=0;i<16;++i) output[offset+i]=char(swapPairs(previous[i]));
    }
    return output.toBase64();
}