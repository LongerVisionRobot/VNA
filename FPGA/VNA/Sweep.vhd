----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    21:35:02 05/06/2020 
-- Design Name: 
-- Module Name:    Sweep - Behavioral 
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

entity Sweep is
    Port ( CLK : in  STD_LOGIC;
           RESET : in  STD_LOGIC;
			  NPOINTS : in STD_LOGIC_VECTOR (12 downto 0);
           CONFIG_ADDRESS : out  STD_LOGIC_VECTOR (12 downto 0);
           CONFIG_DATA : in  STD_LOGIC_VECTOR (95 downto 0);
			  USER_NSAMPLES : in STD_LOGIC_VECTOR (9 downto 0);
			  NSAMPLES : out STD_LOGIC_VECTOR (9 downto 0);
           SAMPLING_BUSY : in STD_LOGIC;
			  SAMPLING_DONE : in  STD_LOGIC;
			  START_SAMPLING : out STD_LOGIC;
			  PORT_SELECT : out STD_LOGIC;
			  BAND_SELECT : out STD_LOGIC;
			  -- fixed part of source/LO registers
           MAX2871_DEF_4 : in  STD_LOGIC_VECTOR (31 downto 0);
           MAX2871_DEF_3 : in  STD_LOGIC_VECTOR (31 downto 0);
           MAX2871_DEF_1 : in  STD_LOGIC_VECTOR (31 downto 0);
           MAX2871_DEF_0 : in  STD_LOGIC_VECTOR (31 downto 0);
			  -- assembled source/LO registers
			  SOURCE_REG_4 : out  STD_LOGIC_VECTOR (31 downto 0);
           SOURCE_REG_3 : out  STD_LOGIC_VECTOR (31 downto 0);
           SOURCE_REG_1 : out  STD_LOGIC_VECTOR (31 downto 0);
           SOURCE_REG_0 : out  STD_LOGIC_VECTOR (31 downto 0);
			  LO_REG_4 : out  STD_LOGIC_VECTOR (31 downto 0);
           LO_REG_3 : out  STD_LOGIC_VECTOR (31 downto 0);
           LO_REG_1 : out  STD_LOGIC_VECTOR (31 downto 0);
           LO_REG_0 : out  STD_LOGIC_VECTOR (31 downto 0);
			  RELOAD_PLL_REGS : out STD_LOGIC;
			  PLL_RELOAD_DONE : in STD_LOGIC;
			  PLL_LOCKED : in STD_LOGIC;
			  SWEEP_HALTED : out STD_LOGIC;
			  SWEEP_RESUME : in STD_LOGIC;
			  
			  ATTENUATOR : out STD_LOGIC_VECTOR(6 downto 0);
			  SOURCE_FILTER : out STD_LOGIC_VECTOR(1 downto 0);
			  
			  --SETTLING_TIME : in STD_LOGIC_VECTOR (15 downto 0);
			  
			  EXCITE_PORT1 : in STD_LOGIC;
			  EXCITE_PORT2 : in STD_LOGIC;
			  
			  -- Debug signals
			  DEBUG_STATUS : out STD_LOGIC_VECTOR (10 downto 0)
			  );
end Sweep;

architecture Behavioral of Sweep is
	signal point_cnt : unsigned(12 downto 0);
	type Point_states is (TriggerSetup, SettingUp, SettlingPort1, ExcitingPort1, SettlingPort2, ExcitingPort2, Done);
	signal state : Point_states;
	signal settling_cnt : unsigned(15 downto 0);
	signal settling_time : unsigned(15 downto 0);
begin
	
	CONFIG_ADDRESS <= std_logic_vector(point_cnt);
	
	-- assemble registers
	-- source register 0: N divider and fractional division value
	SOURCE_REG_0 <= MAX2871_DEF_0(31) & "000000000" & CONFIG_DATA(6 downto 0) & CONFIG_DATA(27 downto 16) & "000";
	-- source register 1: Modulus value
	SOURCE_REG_1 <= MAX2871_DEF_1(31 downto 15) & CONFIG_DATA(39 downto 28) & "001";
	-- source register 3: VCO selection
	SOURCE_REG_3 <= CONFIG_DATA(12 downto 7) & MAX2871_DEF_3(25 downto 3) & "011";
	-- output power A passed on from default registers, output B disabled
	SOURCE_REG_4 <= MAX2871_DEF_4(31 downto 23) & CONFIG_DATA(15 downto 13) & MAX2871_DEF_4(19 downto 9) & "000" & MAX2871_DEF_4(5 downto 3) & "100";
	
	-- LO register 0: N divider and fractional division value
	LO_REG_0 <= MAX2871_DEF_0(31) & "000000000" & CONFIG_DATA(54 downto 48) & CONFIG_DATA(75 downto 64) & "000";
	-- LO register 1: Modulus value
	LO_REG_1 <= MAX2871_DEF_1(31 downto 15) & CONFIG_DATA(87 downto 76) & "001";
	-- LO register 3: VCO selection
	LO_REG_3 <= CONFIG_DATA(60 downto 55) & MAX2871_DEF_3(25 downto 3) & "011";
	-- both outputs enabled at -1dbm
	LO_REG_4 <= MAX2871_DEF_4(31 downto 23) & CONFIG_DATA(63 downto 61) & MAX2871_DEF_4(19 downto 9) & "101101100";
	
	ATTENUATOR <= CONFIG_DATA(46 downto 40);
	SOURCE_FILTER <= CONFIG_DATA(89 downto 88);
	BAND_SELECT <= CONFIG_DATA(47);
	
	settling_time <= 	to_unsigned(2048, 16) when CONFIG_DATA(94 downto 93) = "00" else -- 20us
							to_unsigned(6144, 16) when CONFIG_DATA(94 downto 93) = "01" else -- 60us
							to_unsigned(18432, 16) when CONFIG_DATA(94 downto 93) = "10" else -- 180us
							to_unsigned(55296, 16); -- 540us
							
	NSAMPLES <= USER_NSAMPLES when CONFIG_DATA(92 downto 90) = "000" else
					std_logic_vector(to_unsigned(1, 10)) when CONFIG_DATA(92 downto 90) = "001" else
					std_logic_vector(to_unsigned(3, 10)) when CONFIG_DATA(92 downto 90) = "010" else
					std_logic_vector(to_unsigned(7, 10)) when CONFIG_DATA(92 downto 90) = "011" else
					std_logic_vector(to_unsigned(24, 10)) when CONFIG_DATA(92 downto 90) = "100" else
					std_logic_vector(to_unsigned(71, 10)) when CONFIG_DATA(92 downto 90) = "101" else
					std_logic_vector(to_unsigned(238, 10)) when CONFIG_DATA(92 downto 90) = "110" else
					std_logic_vector(to_unsigned(714, 10));
	
	DEBUG_STATUS(10 downto 8) <= "000" when state = TriggerSetup else
											"001" when state = SettingUp else
											"010" when state = SettlingPort1 else
											"011" when state = ExcitingPort1 else
											"100" when state = SettlingPort2 else
											"101" when state = ExcitingPort2 else
											"110" when state = Done else
											"111";
	DEBUG_STATUS(7) <= PLL_RELOAD_DONE;
	DEBUG_STATUS(6) <= PLL_RELOAD_DONE and PLL_LOCKED;
	DEBUG_STATUS(5) <= SAMPLING_BUSY;
	DEBUG_STATUS(4 downto 0) <= (others => '0');
	
	process(CLK, RESET)
	begin
		if rising_edge(CLK) then
			if RESET = '1' then
				point_cnt <= (others => '0');
				state <= TriggerSetup;
				START_SAMPLING <= '0';
				RELOAD_PLL_REGS <= '0';
				SWEEP_HALTED <= '0';
			else
				case state is
					when TriggerSetup =>
						RELOAD_PLL_REGS <= '1';
						if PLL_RELOAD_DONE = '0' then
							state <= SettingUp;
						end if;
					when SettingUp =>
						-- highest bit in CONFIG_DATA determines whether the sweep should be halted prior to sampling
						SWEEP_HALTED <= CONFIG_DATA(95);
						RELOAD_PLL_REGS <= '0';
						if PLL_RELOAD_DONE = '1' and PLL_LOCKED = '1' then
							-- check if halted sweep is resumed
							if CONFIG_DATA(95) = '0' or SWEEP_RESUME = '1' then
								SWEEP_HALTED <= '0';
								if EXCITE_PORT1 = '1' then
									state <= SettlingPort1;
								elsif EXCITE_PORT2 = '1' then
									state <= SettlingPort2;
								else
									state <= Done;
								end if;
								settling_cnt <= settling_time;
							end if;
						end if;
					when SettlingPort1 =>
						PORT_SELECT <= '1';
						-- wait for settling time to elapse
						if settling_cnt > 0 then
							settling_cnt <= settling_cnt - 1;
						else
							START_SAMPLING <= '1';
							if SAMPLING_BUSY = '1' then
								state <= ExcitingPort1;
							end if;
						end if;
					when ExcitingPort1 =>
						-- wait for sampling to finish
						START_SAMPLING <= '0';
						if SAMPLING_BUSY = '0' then
							if EXCITE_PORT2 = '1' then
								state <= SettlingPort2;
							else
								state <= Done;
							end if;
							settling_cnt <= unsigned(SETTLING_TIME);
						end if;
					when SettlingPort2 =>
						PORT_SELECT <= '0';
						-- wait for settling time to elapse
						if settling_cnt > 0 then
							settling_cnt <= settling_cnt - 1;
						else
							START_SAMPLING <= '1';
							if SAMPLING_BUSY = '1' then
								state <= ExcitingPort2;
							end if;
						end if;
					when ExcitingPort2 =>
						-- wait for sampling to finish
						START_SAMPLING <= '0';
						if SAMPLING_BUSY = '0' then
							if point_cnt < unsigned(NPOINTS) then
								point_cnt <= point_cnt + 1;
								state <= TriggerSetup;
								PORT_SELECT <= '1';
							else 
								point_cnt <= (others => '0');
								state <= Done;
							end if;
						end if;
					when others =>
				end case;
			end if;
		end if;
	end process;
end Behavioral;

