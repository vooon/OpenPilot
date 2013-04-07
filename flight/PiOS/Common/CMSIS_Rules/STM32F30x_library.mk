#
# Rules to add CMSIS specific diectories to a PiOS target
#

EXTRAINCDIRS		+=	$(CMSISDIR)/$(CMSISDEV)
EXTRAINCDIRS		+=	$(CMSISDIR)/$(CMSISCORE)

# Rules to build the ARM DSP library
ifeq ($(USE_DSP_LIB), YES)

DSPLIB_NAME		:=	dsp
CMSIS_DSPLIB		:=	$(EXTLIBS)/$(CMSISDSP)

# Compile all files into output directory
DSPLIB_SRC		:=	$(wildcard $(CMSIS_DSPLIB)/*/*.c)
DSPLIB_SRCBASE		:=	$(notdir $(basename $(DSPLIB_SRC)))
$(foreach src, $(DSPLIB_SRC), $(eval $(call COMPILE_C_TEMPLATE, $(src))))

# Define the object files directory and a list of object files for the library
DSPLIB_OBJDIR		=	$(OUTDIR)
DSPLIB_OBJ		=	$(addprefix $(DSPLIB_OBJDIR)/, $(addsuffix .o, $(DSPLIB_SRCBASE)))

# Create a library file
$(eval $(call ARCHIVE_TEMPLATE, $(OUTDIR)/lib$(DSPLIB_NAME).a, $(DSPLIB_OBJ), $(DSPLIB_OBJDIR)))

# Add library to the list of linked objects
ALLLIB			+=	$(OUTDIR)/lib$(DSPLIB_NAME).a

endif
