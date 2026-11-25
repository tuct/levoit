"""Exercise actual parser and ACK code with corrupted/truncated UART streams."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class UARTIntegrityTests(unittest.TestCase):
    def test_real_stream_parser_and_oem_ack_sequence(self):
        source = ROOT / 'components/levoit'
        text = (source / 'levoit.cpp').read_text()
        parser = text[text.index('            // 1) Read incoming bytes'):text.index('            delay(1);')]
        checked = text[text.index('        void Levoit::process_message('):text.index('        /// @brief Sends the commands depending')].replace('Levoit::process_message(', 'Levoit::process_checked(')
        ack = text[text.index('        void Levoit::ackMessage('):text.index('    } // namespace levoit')]
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            for name in ['types.h', 'levoit_message.h']:
                shutil.copyfile(source / name, p / name)
            (p / 'test.cpp').write_text(r'''
#include <cassert>
#include <cstring>
#include <cstdio>
#include <deque>
#include "types.h"
#include "levoit_message.h"

#define ESP_LOGV(...)
#define ESP_LOGD(...)
#define ESP_LOGI(...)
#define ESP_LOGW(...)
namespace esphome::levoit {
struct Levoit;
void dispatch_decoder(Levoit*,ModelType,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,const uint8_t*,size_t);
struct Levoit {
 ModelType model_=CORE600S;
 bool mcu_seen_=false;
 enum class WifiLedState { UNKNOWN, SOLID };
 WifiLedState wifi_led_state_=WifiLedState::UNKNOWN;
 void cancel_timeout(const char*){}
 int decoded=0;
 void process_checked(uint8_t*,int);
 uint8_t buffer_[512]{};
 size_t buf_len_=0;
 uint32_t last_rx_byte_ms_=0, now=0;
 std::deque<uint8_t> input;
 std::vector<std::vector<uint8_t>> frames;
 std::vector<uint8_t> tx;
 bool available(){return !input.empty();}
 bool read_byte(uint8_t *b){*b=input.front();input.pop_front();return true;}
 void process_message(uint8_t *v,int n){frames.emplace_back(v,v+n);}
 void write_array(const uint8_t *v,size_t n){tx.insert(tx.end(),v,v+n);}
 void flush(){}
 void poll(){
''' + parser + r'''
 }
 void ackMessage(uint8_t,uint8_t,uint8_t);
};
''' + ack + checked + r'''
void dispatch_decoder(Levoit *d,ModelType,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,const uint8_t*,size_t){++d->decoded;}
}
using namespace esphome::levoit;
std::vector<uint8_t> frame(size_t payload) {
 auto f=std::vector<uint8_t>(10+payload,0);
 f[0]=0xa5;f[1]=0x22;f[3]=(payload+4)&255;f[4]=(payload+4)>>8;
 f[6]=1;f[7]=0x40;f[8]=0x41;
 levoit_finalize_message(f,7);return f;
}
void feed(Levoit &d,const std::vector<uint8_t>&v){d.input.insert(d.input.end(),v.begin(),v.end());d.poll();}
int main(){
 // The stock Core trace replies to sequence 0 with exactly these ten bytes.
 Levoit core;messageUpCounter=93;core.ackMessage(0x40,0x41,0);
 assert((core.tx==std::vector<uint8_t>{0xa5,0x52,0,4,0,0x81,1,0x40,0x41,1}));
 assert(messageUpCounter==93);
 Levoit vital;vital.model_=VITAL100S;vital.ackMessage(0,0x55,0xf2);
 assert(vital.tx[2]==0xf2 && vital.tx[4]==0 && vital.tx[6]==2);
 assert(levoit_checksum(vital.tx.data(),vital.tx.size())==vital.tx[5]);
 Levoit guard;auto status=frame(23);guard.process_checked(status.data(),status.size());assert(guard.decoded==1);
 status[6]=2;guard.process_checked(status.data(),status.size());assert(guard.decoded==1);
 status[6]=1;status[9]=1;guard.process_checked(status.data(),status.size());assert(guard.decoded==1);
 guard.model_=VITAL100S;status[9]=0;status[6]=1;guard.process_checked(status.data(),status.size());assert(guard.decoded==1);
 status[6]=2;guard.process_checked(status.data(),status.size());assert(guard.decoded==2);
 Levoit d;auto good=frame(23);feed(d,good);assert(d.frames.size()==1);
 // Correct 16-bit length, split at a byte boundary between bursts.
 auto long_frame=frame(300);auto first=std::vector<uint8_t>(long_frame.begin(),long_frame.begin()+200);
 feed(d,first);assert(d.frames.size()==1);d.now=200;
 feed(d,{long_frame.begin()+200,long_frame.end()});assert(d.frames.size()==2 && d.frames.back().size()==310);
 // A stuck length is discarded before the next burst after 250ms silence.
 feed(d,{0xa5,0x22,0,0xf0,0,0});d.now+=251;feed(d,good);assert(d.frames.size()==3);
 // Large high byte and impossible short lengths must resynchronize immediately.
 for(auto junk:{std::vector<uint8_t>{0xa5,0x22,0,4,0xff,0},std::vector<uint8_t>{0xa5,0x22,0,0,0,0}}){
  junk.insert(junk.end(),good.begin(),good.end());auto old=d.frames.size();feed(d,junk);assert(d.frames.size()==old+1);
 }
 auto corrupt=good;corrupt[5]^=1;corrupt.insert(corrupt.end(),good.begin(),good.end());
 auto old=d.frames.size();feed(d,corrupt);assert(d.frames.size()==old+1);
 Levoit wrap;wrap.now=0xfffffff0;feed(wrap,{0xa5,0x22});wrap.now=300;feed(wrap,good);assert(wrap.frames.size()==1);
}
''')
            exe = p / 'uart-integrity'
            subprocess.run([shutil.which('clang++') or 'c++', '-std=c++20',
                            '-fsanitize=' + os.environ.get('LEVOIT_TEST_SANITIZERS', 'address,undefined'),
                            '-I', str(p), str(p / 'test.cpp'), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)


if __name__ == '__main__': unittest.main()
