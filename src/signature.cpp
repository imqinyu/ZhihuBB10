// SPDX-License-Identifier: AGPL-3.0-or-later
// Adapted from Zhihu++ ZseSigner.kt / ZhihuFetchSignature.kt.
// Copyright (C) 2024-2026 zly2006 <i@zly2006.me>. See THIRD_PARTY_NOTICES.md.
#include "signature.hpp"
#include <QCryptographicHash>
static const quint32 ZK[] = {1170614578u,1024848638u,1413669199u,3951632832u,3528873006u,2921909214u,4151847688u,3997739139u,1933479194u,3323781115u,3888513386u,460404854u,3747539722u,2403641034u,2615871395u,2119585428u,2265697227u,2035090028u,2773447226u,4289380121u,4217216195u,2200601443u,3051914490u,1579901135u,1321810770u,456816404u,2903323407u,4065664991u,330002838u,3506006750u,363569021u,2347096187u};
static const quint32 ZB[] = {20u,223u,245u,7u,248u,2u,194u,209u,87u,6u,227u,253u,240u,128u,222u,91u,237u,9u,125u,157u,230u,93u,252u,205u,90u,79u,144u,199u,159u,197u,186u,167u,39u,37u,156u,198u,38u,42u,43u,168u,217u,153u,15u,103u,80u,189u,71u,191u,97u,84u,247u,95u,36u,69u,14u,35u,12u,171u,28u,114u,178u,148u,86u,182u,32u,83u,158u,109u,22u,255u,94u,238u,151u,85u,77u,124u,254u,18u,4u,26u,123u,176u,232u,193u,131u,172u,143u,142u,150u,30u,10u,146u,162u,62u,224u,218u,196u,229u,1u,192u,213u,27u,110u,56u,231u,180u,138u,107u,242u,187u,54u,120u,19u,44u,117u,228u,215u,203u,53u,239u,251u,127u,81u,11u,133u,96u,204u,132u,41u,115u,73u,55u,249u,147u,102u,48u,122u,145u,106u,118u,74u,190u,29u,16u,174u,5u,177u,129u,63u,113u,99u,31u,161u,76u,246u,34u,211u,13u,60u,68u,207u,160u,65u,111u,82u,165u,67u,169u,225u,57u,112u,244u,155u,51u,236u,200u,233u,58u,61u,47u,100u,137u,185u,64u,17u,70u,234u,163u,219u,108u,170u,166u,59u,149u,52u,105u,24u,212u,78u,173u,45u,0u,116u,226u,119u,136u,206u,135u,175u,195u,25u,92u,121u,208u,126u,139u,3u,75u,141u,21u,130u,98u,241u,40u,154u,66u,184u,49u,181u,46u,243u,88u,101u,183u,8u,23u,72u,188u,104u,179u,210u,134u,250u,201u,164u,89u,216u,202u,220u,50u,221u,152u,140u,33u,235u,214u};
static quint32 rotate(quint32 x, int n) { return (x << n) | (x >> (32-n)); }
static QByteArray block(const QByteArray &input) {
    quint32 w[36];
    for (int i=0; i<4; ++i) {
        w[i]=0;
        for (int j=0; j<4; ++j) w[i]=(w[i]<<8) | quint8(input[i*4+j]);
    }
    for (int i=0; i<32; ++i) {
        quint32 n=w[i+1]^w[i+2]^w[i+3]^ZK[i];
        quint32 t=(ZB[(n>>24)&255]<<24)|(ZB[(n>>16)&255]<<16)|(ZB[(n>>8)&255]<<8)|ZB[n&255];
        w[i+4]=w[i]^t^rotate(t,2)^rotate(t,10)^rotate(t,18)^rotate(t,24);
    }
    QByteArray out(16,0);
    for (int i=0; i<4; ++i)
        for (int j=0; j<4; ++j) out[i*4+j]=char(w[35-i]>>(24-j*8));
    return out;
}
QByteArray zhihuSignature(const QUrl &url, const QByteArray &dc0) {
    QByteArray path=url.toEncoded(QUrl::RemoveScheme|QUrl::RemoveAuthority|QUrl::RemoveFragment);
    QByteArray hash=QCryptographicHash::hash("101_3_3.0+"+path+"+"+dc0,QCryptographicHash::Md5).toHex();
    QByteArray plain; plain.append(char(210)); plain.append(char(0)); plain.append(hash);
    int pad=16-plain.size()%16; plain.append(QByteArray(pad,char(pad)));
    const QByteArray key("059053f7d15e01d7");
    QByteArray first=plain.left(16);
    for (int i=0; i<16; ++i) first[i]=char(quint8(first[i])^quint8(key[i])^42);
    QByteArray iv=block(first), cipher=iv;
    for (int pos=16; pos<plain.size(); pos+=16) {
        QByteArray part=plain.mid(pos,16);
        for (int i=0; i<16; ++i) part[i]=char(quint8(part[i])^quint8(iv[i]));
        iv=block(part); cipher+=iv;
    }
    while (cipher.size()%3) cipher.append(char(0));
    const QByteArray alphabet("6fpLRqJO8M/c3jnYxFkUVC4ZIG12SiH=5v0mXDazWBTsuw7QetbKdoPyAl+hN9rgE");
    QByteArray out("2.0_"); int index=0;
    for (int pos=cipher.size()-1; pos>=0; pos-=3) {
        quint32 v=0;
        for (int i=0; i<3; ++i) {
            v|=(quint8(cipher[pos-i])^(index%4==0 ? 58:0))<<(i*8); ++index;
        }
        for (int shift=0; shift<24; shift+=6) out.append(alphabet[(v>>shift)&63]);
    }
    return out;
}
