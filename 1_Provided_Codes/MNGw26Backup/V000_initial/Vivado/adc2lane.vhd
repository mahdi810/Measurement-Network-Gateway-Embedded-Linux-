----------------------------------------------------------------------------------
-- Company:         Univ. Bremerhaven
-- Engineer:        Kai Mueller
-- 
-- Create Date:     08.06.2016 00:15:32
-- Description:     ADC 2 Lanes SPI based on spipol1pha0 template (CPOL=0 CPHA=1)
-- 
----------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity adc2lane is
    GENERIC ( USPI_SIZE : INTEGER := 16 );
    Port ( resetn : in STD_LOGIC;
       bclk : in STD_LOGIC;
       spi_clkp : in STD_LOGIC;
       start : in STD_LOGIC;
       done : out STD_LOGIC;
       scsq : out STD_LOGIC;
       sclk : out STD_LOGIC;
       sdi_0 : in STD_LOGIC;
       sdi_1 : in STD_LOGIC;
       rcvData_0 : out STD_LOGIC_VECTOR (USPI_SIZE-1 downto 0);
       rcvData_1 : out STD_LOGIC_VECTOR (USPI_SIZE-1 downto 0));
end adc2lane;

architecture Behavioral of adc2lane is

type state_type IS (sidle, sstartx, sclk_lo, sclk_hi, sstop_lo, sstop_hi);
SIGNAL  state, next_state: state_type;
SIGNAL sclk_i, scsq_i : STD_LOGIC;
SIGNAL rd_buf_0, rd_buf_1 : STD_LOGIC_VECTOR(USPI_SIZE-1 downto 0);
SIGNAL count : integer RANGE 0 TO USPI_SIZE-1;

type sslst_type IS (ssl_idle, ssl_start, ssl_run);
SIGNAL ssl_state, sslnxt_state: sslst_type;
SIGNAL start_i : STD_LOGIC;

begin

    rcvData_0 <= rd_buf_0;
    rcvData_1 <= rd_buf_1;

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
	sseq_proc: PROCESS(resetn, bclk, next_state, sclk_i, scsq_i,
				       rd_buf_0, rd_buf_0, sdi_0, sdi_1, count, spi_clkp)
	BEGIN
		IF rising_edge(bclk) THEN
			IF resetn='0' THEN
				state <= sidle;
				count <= USPI_SIZE-1;
			ELSIF spi_clkp='1' THEN
				IF next_state=sstartx THEN
                count <= USPI_SIZE - 1;
                ELSIF next_state=sclk_lo THEN
                    count <= count - 1;
                    rd_buf_0 <= rd_buf_0(USPI_SIZE-2 downto 0) & sdi_0;
                    rd_buf_1 <= rd_buf_1(USPI_SIZE-2 downto 0) & sdi_1;
                ELSIF next_state=sstop_lo THEN
                    rd_buf_0 <= rd_buf_0(USPI_SIZE-2 downto 0) & sdi_0;
                    rd_buf_1 <= rd_buf_1(USPI_SIZE-2 downto 0) & sdi_1;
                END IF;
                state <= next_state;
                sclk <= sclk_i;
                scsq <= scsq_i;
			END IF;
		END IF;
	END PROCESS sseq_proc;

	scmb_proc: PROCESS(state, start_i, count)
	BEGIN
		next_state <= state;
		sclk_i <= '1';
		scsq_i <= '0';
		CASE state IS
			WHEN sidle =>
				scsq_i <= '1';
				IF start_i='1' THEN
					next_state <= sstartx;
				END IF;
			WHEN sstartx =>
                next_state <= sclk_hi;
			WHEN sclk_hi =>
                sclk_i <= '0';
                IF count=0 THEN
                    next_state <= sstop_lo;
                ELSE
                    next_state <= sclk_lo;
                END IF;
            WHEN sclk_lo =>
				next_state <= sclk_hi;
			WHEN sstop_lo =>
				next_state <= sstop_hi;
			WHEN sstop_hi =>
				scsq_i <= '1';
				next_state <= sidle;
		END CASE;
	END PROCESS scmb_proc;

end Behavioral;
