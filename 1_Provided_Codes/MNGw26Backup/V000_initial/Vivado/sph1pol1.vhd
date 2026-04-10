----------------------------------------------------------------------------------
-- Company:         Univ. Bremerhaven
-- Engineer:        Kai Mueller
-- 
-- Create Date:     10.11.2021 22:55
-- Description:     MAX PMB1# SPI (CPOL=1 CPHA=1)
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity sph1pol1 is
    GENERIC ( USPI_SIZE : INTEGER := 16 );
    Port ( resetn : in STD_LOGIC;
            bclk : in STD_LOGIC;
            spi_clkp : in STD_LOGIC;
            start : in STD_LOGIC;
            done : out STD_LOGIC;
            scsq : out STD_LOGIC;
            sclk : out STD_LOGIC;
            sdo : out STD_LOGIC;
            sdi : in STD_LOGIC;
            sndData : in STD_LOGIC_VECTOR (USPI_SIZE-1 downto 0);
            rcvData : out STD_LOGIC_VECTOR (USPI_SIZE-1 downto 0));
end sph1pol1;

architecture Behavioral of sph1pol1 is

type state_type IS (sidle, sstartx, sstart_lo, sclk_lo, sclk_hi, stop_hi, stop_lo);
SIGNAL  state, next_state: state_type;
SIGNAL sclk_i, scsq_i, sdo_i : STD_LOGIC;
SIGNAL wr_buf : STD_LOGIC_VECTOR(USPI_SIZE-1 downto 0);
SIGNAL rd_buf : STD_LOGIC_VECTOR(USPI_SIZE-1 downto 0);
SIGNAL count : integer RANGE 0 TO USPI_SIZE-1;

type sslst_type IS (ssl_idle, ssl_start, ssl_run);
SIGNAL ssl_state, sslnxt_state: sslst_type;
SIGNAL start_i : STD_LOGIC;

begin

	rcvData <= rd_buf;

	-- start/stop logic
	-- proper handling of clock division
	sslseq_proc: PROCESS(resetn, bclk, sslnxt_state)
	BEGIN
		IF rising_edge(bclk) THEN
			IF resetn='0' THEN
				ssl_state <= ssl_idle;
			ELSE
				ssl_state <= sslnxt_state;
			END IF;
		END IF;
	END PROCESS sslseq_proc;

	sslcmb_proc: PROCESS(ssl_state, state, start)
	BEGIN
		sslnxt_state <= ssl_state;
		start_i <= '0';
		done <= '0';
		CASE ssl_state IS
			WHEN ssl_idle =>
				done <= '1';
				IF start='1' AND state=sidle THEN
					sslnxt_state <= ssl_start;
				END IF;
			WHEN ssl_start =>
				start_i <= '1';
				IF state=sstartx THEN
					sslnxt_state <= ssl_run;
				END IF;
			WHEN ssl_run =>
				IF state=sidle THEN
					sslnxt_state <= ssl_idle;
				END IF;
		END CASE;
	END PROCESS sslcmb_proc;

	-- spi logic
	sseq_proc: PROCESS(bclk)
	BEGIN
		IF rising_edge(bclk) THEN
			IF resetn='0' THEN
				state <= sidle;
				count <= USPI_SIZE-1;
			ELSIF spi_clkp='1' THEN
				IF next_state=sstartx THEN
                wr_buf <= sndData;
                count <= USPI_SIZE - 1;
                ELSIF next_state=sclk_lo THEN
                    wr_buf <= wr_buf(USPI_SIZE-2 downto 0) & '-';
                    rd_buf <= rd_buf(USPI_SIZE-2 downto 0) & sdi;
                ELSIF next_state=sclk_hi THEN
                    count <= count - 1;
                ELSIF next_state=stop_lo THEN
                    rd_buf <= rd_buf(USPI_SIZE-2 downto 0) & sdi;
                END IF;
                state <= next_state;
                sclk <= sclk_i;
                scsq <= scsq_i;
                sdo <= sdo_i;
			END IF;
		END IF;
	END PROCESS sseq_proc;

	scmb_proc: PROCESS(state, start_i, count, wr_buf)
	BEGIN
		next_state <= state;
		sclk_i <= '1';
		scsq_i <= '0';
		sdo_i <= '0';
		CASE state IS
			WHEN sidle =>
				scsq_i <= '1';
				IF start_i='1' THEN
					next_state <= sstartx;
				END IF;
			WHEN sstartx =>
				next_state <= sstart_lo;
			WHEN sstart_lo =>
				sclk_i <= '0';
				sdo_i <= wr_buf(USPI_SIZE-1);
				next_state <= sclk_hi;
			WHEN sclk_hi =>
				sdo_i <= wr_buf(USPI_SIZE-1);
				next_state <= sclk_lo;
			WHEN sclk_lo =>
				sclk_i <= '0';
				sdo_i <= wr_buf(USPI_SIZE-1);
				IF count=0 THEN
					next_state <= stop_hi;
				ELSE
					next_state <= sclk_hi;
				END IF;
			WHEN stop_hi =>
				sdo_i <= wr_buf(USPI_SIZE-1);
				next_state <= stop_lo;
			WHEN stop_lo =>
				scsq_i <= '1';
				next_state <= sidle;
		END CASE;
	END PROCESS scmb_proc;

end Behavioral;
