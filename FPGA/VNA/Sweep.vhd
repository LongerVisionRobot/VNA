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
           CONFIG_DATA : in  STD_LOGIC_VECTOR (111 downto 0);
           SAMPLING_DONE : in  STD_LOGIC;
			  START_SAMPLING : out STD_LOGIC;
			  PORT_SELECT : out STD_LOGIC;
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
			  
			  ATTENUATOR : out STD_LOGIC_VECTOR(6 downto 0);
			  SOURCE_FILTER : out STD_LOGIC_VECTOR(1 downto 0);
			  
			  SETTLING_TIME : in STD_LOGIC_VECTOR (15 downto 0));
end Sweep;

architecture Behavioral of Sweep is
	signal point_cnt : unsigned(12 downto 0);
	type Point_states is (TriggerSetup, SettingUp, SettlingPort1, ExcitingPort1, SettlingPort2, ExcitingPort2, Done);
	signal state : Point_states;
	signal settling_cnt : unsigned(15 downto 0);
begin
	
	CONFIG_ADDRESS <= std_logic_vector(point_cnt);
	
	-- assemble registers
	-- sweep config content:
	-- 15 downto 0: Source N divider
	-- 27 downto 16: Source Frac
	-- 39 downto 28: Source M
	-- 45 downto 40: Source VCO selection
	-- 48 downto 46: Source DIVA
	-- 55 downto 49: Attenuator selection
	-- 71 downto 56: LO N divider
	-- 83 downto 72: LO Frac
	-- 95 downto 84: LO M
	-- 101 downto 96: LO VCO selection
	-- 104 downto 102: LO DIVA
	-- 106 downto 105: Source filter selection
	-- 111 downto 107: reserved
	SOURCE_REG_0 <= MAX2871_DEF_0(31) & CONFIG_DATA(15 downto 0) & CONFIG_DATA(27 downto 16) & "000";
	SOURCE_REG_1 <= MAX2871_DEF_1(31 downto 15) & CONFIG_DATA(39 downto 28) & "001";
	SOURCE_REG_3 <= CONFIG_DATA(45 downto 40) & MAX2871_DEF_3(25 downto 3) & "011";
	SOURCE_REG_4 <= MAX2871_DEF_4(31 downto 23) & CONFIG_DATA(48 downto 46) & MAX2871_DEF_4(19 downto 3) & "100";
	
	LO_REG_0 <= MAX2871_DEF_0(31) & CONFIG_DATA(71 downto 56) & CONFIG_DATA(83 downto 72) & "000";
	LO_REG_1 <= MAX2871_DEF_1(31 downto 15) & CONFIG_DATA(95 downto 84) & "001";
	LO_REG_3 <= CONFIG_DATA(101 downto 96) & MAX2871_DEF_3(25 downto 3) & "011";
	LO_REG_4 <= MAX2871_DEF_4(31 downto 23) & CONFIG_DATA(104 downto 102) & MAX2871_DEF_4(19 downto 3) & "100";
	
	ATTENUATOR <= CONFIG_DATA(55 downto 49);
	SOURCE_FILTER <= CONFIG_DATA(106 downto 105);
	
	process(CLK, RESET)
	begin
		if rising_edge(CLK) then
			if RESET = '1' then
				point_cnt <= (others => '0');
				state <= TriggerSetup;
				START_SAMPLING <= '0';
				PORT_SELECT <= '1';
				RELOAD_PLL_REGS <= '0';
			else
				case state is
					when TriggerSetup =>
						RELOAD_PLL_REGS <= '1';
						if PLL_RELOAD_DONE = '0' then
							state <= SettingUp;
						end if;
					when SettingUp =>
						RELOAD_PLL_REGS <= '0';
						if PLL_RELOAD_DONE = '1' and PLL_LOCKED = '1' then
							state <= SettlingPort1;
							settling_cnt <= unsigned(SETTLING_TIME);
							PORT_SELECT <= '1';
						end if;
					when SettlingPort1 =>
						-- wait for settling time to elapse
						if settling_cnt > 0 then
							settling_cnt <= settling_cnt - 1;
						else
							state <= ExcitingPort1;
							START_SAMPLING <= '1';
						end if;
					when ExcitingPort1 =>
						-- wait for sampling to finish
						START_SAMPLING <= '0';
						if SAMPLING_DONE = '1' then
							state <= SettlingPort2;
							PORT_SELECT <= '0';
							settling_cnt <= unsigned(SETTLING_TIME);
						end if;
					when SettlingPort2 =>
						-- wait for settling time to elapse
						if settling_cnt > 0 then
							settling_cnt <= settling_cnt - 1;
						else
							state <= ExcitingPort2;
							START_SAMPLING <= '1';
						end if;
					when ExcitingPort2 =>
						-- wait for sampling to finish
						START_SAMPLING <= '0';
						if SAMPLING_DONE = '1' then
							if point_cnt < unsigned(NPOINTS) then
								point_cnt <= point_cnt + 1;
								state <= TriggerSetup;
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

