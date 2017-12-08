PROJECT=smartsdr-dsp
LDFLAGS=-lm
C2DIR=CODEC2_FREEDV
SSIDIR=SmartSDR_Interface

CFLAGS=-O3 -Wall -I$(SSIDIR) -I$(C2DIR) -I.
CC=arm-linux-musleabihf-gcc
#CC=gcc

SOURCES=$(C2DIR)/c2wideband.c \
$(C2DIR)/codebook.c \
$(C2DIR)/codebookd.c \
$(C2DIR)/codebookdt.c \
$(C2DIR)/codebookge.c \
$(C2DIR)/codebookjvm.c \
$(C2DIR)/codebooklspmelvq.c \
$(C2DIR)/codebookmel.c \
$(C2DIR)/codebooknewamp1.c \
$(C2DIR)/codebooknewamp1_energy.c \
$(C2DIR)/codebookres.c \
$(C2DIR)/codebookvq.c \
$(C2DIR)/codec2.c \
$(C2DIR)/codec2_fft.c \
$(C2DIR)/cohpsk.c \
$(C2DIR)/dct2.c \
$(C2DIR)/dump.c \
$(C2DIR)/fdmdv.c \
$(C2DIR)/fifo.c \
$(C2DIR)/fmfsk.c \
$(C2DIR)/freedv_api.c \
$(C2DIR)/freedv_data_channel.c \
$(C2DIR)/freedv_vhf_framing.c \
$(C2DIR)/fsk.c \
$(C2DIR)/golay23.c \
$(C2DIR)/interp.c \
$(C2DIR)/kiss_fft.c \
$(C2DIR)/kiss_fftr.c \
$(C2DIR)/linreg.c \
$(C2DIR)/lpc.c \
$(C2DIR)/lsp.c \
$(C2DIR)/mbest.c \
$(C2DIR)/modem_probe.c \
$(C2DIR)/modem_stats.c \
$(C2DIR)/newamp1.c \
$(C2DIR)/nlp.c \
$(C2DIR)/octave.c \
$(C2DIR)/pack.c \
$(C2DIR)/phase.c \
$(C2DIR)/postfilter.c \
$(C2DIR)/quantise.c \
$(C2DIR)/sine.c \
$(C2DIR)/varicode.c \
$(SSIDIR)/cmd_basics.c \
$(SSIDIR)/cmd_engine.c \
$(SSIDIR)/discovery_client.c \
$(SSIDIR)/hal_buffer.c \
$(SSIDIR)/hal_listener.c \
$(SSIDIR)/hal_vita.c \
$(SSIDIR)/io_utils.c \
$(SSIDIR)/sched_waveform.c \
$(SSIDIR)/smartsdr_dsp_api.c \
$(SSIDIR)/status_processor.c \
$(SSIDIR)/traffic_cop.c \
$(SSIDIR)/utils.c \
$(SSIDIR)/vita_output.c \
circular_buffer.c \
main.c \
resampler.c


OBJECTS = $(SOURCES:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

objs: $(OBJECTS)

freedv: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -static -o $@

clean:
	rm -f $(OBJECTS) freedv

all: freedv