/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

/* Some Low-level definitions for STIM DA-converter. */

#define DACZERO		(2048)	/* Zero-level for 12-bit DAC. */

#define DA_NORM		(647)	/* Reference Audio Level */
#define AMP_L20		(204)	/*  -20dB of Reference */
#define AMP_H20		(2045)	/*  +20dB of Reference */

#define PHASE_LL	(50)	/* Left  is leading */
#define PHASE_RL	(-50)	/* Right is leading */

#define TRIG_HI_STEPS		(1) /* One period in 50KHz for trigger. */

/* Test and paradigm codes for switching. */
#define TEST_CALIBRATION	(0x0001)
#define TEST_SINECOSINE		(0x0002)
#define TEST_SQUARE		(0x0003)
#define TEST_STEADYSQUARE	(0x0004)
#define PARA_CLICK		(0x1001)
#define PARA_SQUAREBURST	(0x1002)
#define PARA_IIDITD		(0x1003)
#define PARA_IIDITD_PPLP	(0x1004)
