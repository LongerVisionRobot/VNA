----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    17:27:54 05/05/2020 
-- Design Name: 
-- Module Name:    Sampling - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity Sampling is
	 Generic(CLK_DIV : integer;
				CLK_FREQ : integer;
				IF_FREQ : integer;
				CLK_CYCLES_PRE_DONE : integer);
    Port ( CLK : in  STD_LOGIC;
           RESET : in  STD_LOGIC;
           PORT1 : in  STD_LOGIC_VECTOR (15 downto 0);
           PORT2 : in  STD_LOGIC_VECTOR (15 downto 0);
           REF : in  STD_LOGIC_VECTOR (15 downto 0);
			  ADC_START : out STD_LOGIC;
           NEW_SAMPLE : in  STD_LOGIC;
           DONE : out  STD_LOGIC;
           PRE_DONE : out  STD_LOGIC;
           START : in  STD_LOGIC;
           SAMPLES : in  STD_LOGIC_VECTOR (16 downto 0);
           PORT1_I : out  STD_LOGIC_VECTOR (47 downto 0);
           PORT1_Q : out  STD_LOGIC_VECTOR (47 downto 0);
           PORT2_I : out  STD_LOGIC_VECTOR (47 downto 0);
           PORT2_Q : out  STD_LOGIC_VECTOR (47 downto 0);
           REF_I : out  STD_LOGIC_VECTOR (47 downto 0);
           REF_Q : out  STD_LOGIC_VECTOR (47 downto 0));
end Sampling;

architecture Behavioral of Sampling is
COMPONENT SinCos
  PORT (
    clk : IN STD_LOGIC;
    phase_in : IN STD_LOGIC_VECTOR(11 DOWNTO 0);
    cosine : OUT STD_LOGIC_VECTOR(15 DOWNTO 0);
    sine : OUT STD_LOGIC_VECTOR(15 DOWNTO 0)
  );
END COMPONENT;
COMPONENT SinCosMult
  PORT (
    clk : IN STD_LOGIC;
    a : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
    b : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
    p : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
  );
END COMPONENT;

	signal p1_I : signed(47 downto 0);
	signal p1_Q : signed(47 downto 0);
	signal p2_I : signed(47 downto 0);
	signal p2_Q : signed(47 downto 0);
	signal r_I : signed(47 downto 0);
	signal r_Q : signed(47 downto 0);
	signal clk_cnt : integer range 0 to CLK_DIV - 1;
	signal sample_cnt : integer range 0 to 131071;
	signal busy : std_logic;
	
	constant phase_inc : integer := IF_FREQ * 4096 * CLK_DIV / CLK_FREQ;
	signal phase : std_logic_vector(11 downto 0);
	signal sine : std_logic_vector(15 downto 0);
	signal cosine : std_logic_vector(15 downto 0);
	
	signal mult1_I : std_logic_vector(31 downto 0);
	signal mult1_Q : std_logic_vector(31 downto 0);
	signal mult2_I : std_logic_vector(31 downto 0);
	signal mult2_Q : std_logic_vector(31 downto 0);
	signal multR_I : std_logic_vector(31 downto 0);
	signal multR_Q : std_logic_vector(31 downto 0);
	
	-- delay line to adjust for mulitplier latency
	signal mult_delay : std_logic_vector(2 downto 0);
	
	signal sampling_done : std_logic;
	signal result_computed : std_logic;
begin
--	assert (phase_inc * CLK_FREQ / (4096*CLK_DIV) = IF_FREQ)
--		report "Phase increment not exact"
--		severity FAILURE;
		
	LookupTable : SinCos
	PORT MAP (
		clk => CLK,
		phase_in => phase,
		cosine => cosine,
		sine => sine
	);
	Port1_I_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => PORT1,
		b => cosine,
		p => mult1_I
	);
	Port1_Q_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => PORT1,
		b => sine,
		p => mult1_Q
	);
	Port2_I_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => PORT2,
		b => cosine,
		p => mult2_I
	);
	Port2_Q_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => PORT2,
		b => sine,
		p => mult2_Q
	);
	Ref_I_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => REF,
		b => cosine,
		p => multR_I
	);
	Ref_Q_Mult : SinCosMult
	PORT MAP (
		clk => CLK,
		a => REF,
		b => sine,
		p => multR_Q
	);
		
	DONE <= result_computed ;
	
	process(CLK, RESET)
	begin
		if rising_edge(CLK) then
			if RESET = '1' then
				busy <= '0';
				ADC_START <= '0';
			else
				if sampling_done = '1' then
					sampling_done <= '0';
					result_computed <= '1';
					PORT1_I <= std_logic_vector(p1_I);
					PORT1_Q <= std_logic_vector(p1_Q);
					PORT2_I <= std_logic_vector(p2_I);
					PORT2_Q <= std_logic_vector(p2_Q);
					REF_I <= std_logic_vector(r_I);
					REF_Q <= std_logic_vector(r_Q);
				end if;
				if result_computed = '1' then
					result_computed <= '0';
				end if;
				if busy = '1' then
					mult_delay <= mult_delay(1 downto 0) & NEW_SAMPLE;
					-- keep track of timing for starting the samples
					if clk_cnt = CLK_DIV - 1 then
						ADC_START <= '1';
						clk_cnt <= 0;
					else
						clk_cnt <= clk_cnt + 1;
						ADC_START <= '0';
					end if;
					-- save completed samples
					if mult_delay(2) = '1' then
						-- multipliers are finished with the sample
						p1_I <= p1_I + signed(mult1_I);
						p1_Q <= p1_Q + signed(mult1_Q);
						p2_I <= p2_I + signed(mult2_I);
						p2_Q <= p2_Q + signed(mult2_Q);
						r_I <= r_I + signed(multR_I);
						r_Q <= r_Q + signed(multR_Q);
						-- advance phase
						phase <= std_logic_vector(unsigned(phase) + phase_inc);
						if sample_cnt < unsigned(SAMPLES) - 1 then
							sample_cnt <= sample_cnt + 1;
						else
							-- sampling done
							busy <= '0';
							sampling_done <= '1';
						end if;
					end if;
				elsif START = '1' then
					busy <= '1';
					p1_I <= (others => '0');
					p1_Q <= (others => '0');
					p2_I <= (others => '0');
					p2_Q <= (others => '0');
					r_I <= (others => '0');
					r_Q <= (others => '0');
					phase <= (others => '0');
					sample_cnt <= 0;
					clk_cnt <= 0;
					mult_delay <= (others => '0');
					-- start the acquisition of the first sample
					ADC_START <= '1';
				end if;
			end if;
		end if;
	end process;

end Behavioral;

