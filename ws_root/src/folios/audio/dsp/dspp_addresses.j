\ @(#) dspp_addresses.j 96/03/07 1.6
\ $Id: dspp_addresses.j,v 1.9 1995/05/17 02:41:07 phil Exp $
\ Copyright (C) 1992, 3DO Company.
\ All Rights Reserved
\ Confidential and Proprietary

\ These are the DSPP addresses for memory mapped I/O
\ in the DSPP data memory.

decimal

include? [if] cond_comp.fth

anew task-dspp_addresses.j

exists? BDA_PASS not [IF]
	2 constant BDA_PASS  \ change for Pass 1 or 2 silicon !!!
[else]
	.( use predefined BDA_PASS) cr
[then]

." ----------- dspp_addresses.j ---------- BDA Pass " BDA_PASS . cr


$ 40000000 constant SDRAM_BASE
$ 00060000 constant DSPP_BASE


$ 1000 constant DSPI_START              
$ 1FFF constant DSPI_END                
$ 1300 constant DSPI_IO_LOW             
$ 13FA constant DSPI_IO_HIGH            

$ 0300 constant DSPI_FIFO_OSC              

$ 00 constant DSPI_FIFO_CURRENT_OFFSET                 \ OFFSETS IN FIFO_OSC FOR EACH CHANNEL */
$ 01 constant DSPI_FIFO_NEXT_OFFSET      
$ 02 constant DSPI_FIFO_FREQUENCY_OFFSET 
$ 03 constant DSPI_FIFO_PHASE_OFFSET     

8 constant DSPP_FIFO_NUM_SAMPLES       \ number of samples in FIFO

BDA_PASS 1 =
[if]   \ ----- ----- ----- ----- ----- ----- ----- ----- PASS 1

24 constant DSPI_NUM_CHANNELS          

$ 04 constant DSPI_FIFO_OSC_SIZE         

$ 0380 constant DSPI_FIFO_BUMP             

$ 00 constant DSPI_FIFO_BUMP_CURR_OFFSET               \ OFFSETS IN CURR_BUMP FOR EACH CHANNEL */ 
$ 01 constant DSPI_FIFO_BUMP_STATUS_OFFSET   
$ 02 constant DSPI_FIFO_BUMP_SIZE        

$ 03C0 constant DSPI_INPUT0                
$ 03C1 constant DSPI_INPUT1                
$ 03C8 constant DSPI_OUTPUT0               
$ 03C9 constant DSPI_OUTPUT1               
$ 03CA constant DSPI_OUTPUT2               
$ 03CB constant DSPI_OUTPUT3               
$ 03CC constant DSPI_OUTPUT4               
$ 03CD constant DSPI_OUTPUT5               
$ 03CE constant DSPI_OUTPUT6               
$ 03CF constant DSPI_OUTPUT7               
$ 03D0 constant DSPI_INPUT_CONTROL         
$ 03D1 constant DSPI_OUTPUT_CONTROL        
$ 03D2 constant DSPI_INPUT_STATUS          
$ 03D3 constant DSPI_OUTPUT_STATUS       

$ 03E0 constant DSPI_PC                         

$ 03EC constant DSPI_SIM_ADIO_FILE   \ adio control for 'C' simulator, not in hardware 

$ 03F0 constant DSPI_CPU_INT0              
$ 03F1 constant DSPI_CPU_INT1   

$ 03F2 constant DSPI_AUDLOCK               
$ 03F9 constant DSPI_CLOCK                 
$ 03FA constant DSPI_NOISE     
$ 03FE constant DSPI_TRACE_SIM   \ trace control for 'C' simulator, not in hardware   
    
$ 0400 constant DSPXB_INT_SOFT0        
$ 0200	constant DSPXB_INT_SOFT1        


DSPP_BASE $ 4030 + constant DSPX_INT_DMALATE_SET        
DSPP_BASE $ 4034 + constant DSPX_INT_DMALATE_CLR        
DSPP_BASE $ 4038 + constant DSPX_INT_DMALATE_ENABLE     
DSPP_BASE $ 403C + constant DSPX_INT_DMALATE_DISABLE    
DSPP_BASE $ 4040 + constant DSPX_INT_UNDEROVER_SET      
DSPP_BASE $ 4044 + constant DSPX_INT_UNDEROVER_CLR      
DSPP_BASE $ 4048 + constant DSPX_INT_UNDEROVER_ENABLE   
DSPP_BASE $ 404C + constant DSPX_INT_UNDEROVER_DISABLE  

DSPI_FIFO_OSC constant DSPI_MAX_DATA_ADDRESS


DSPP_BASE $ 5200 + constant DSPX_DMA_STACK_CONTROL
$ 4 constant DSPX_DMA_STACK_CONTROL_SIZE
$ 0001 constant DSPXB_DMA_NEXTVALID    
$ 0002 constant DSPXB_DMA_GO_FOREVER
$ 0004 constant DSPXB_INT_DMANEXT_EN   
$ 80000000 constant DSPXB_SHADOW_NEXT_ADDRESS
$ 40000000 constant DSPXB_SHADOW_NEXT_COUNT

[else] \ ----- ----- ----- ----- ----- ----- ----- -----  PASS 2
32 constant DSPI_NUM_CHANNELS          

$ 04 constant DSPI_FIFO_BUMP_CURR_OFFSET               \ OFFSETS IN CURR_BUMP FOR EACH CHANNEL */ 
$ 05 constant DSPI_FIFO_BUMP_STATUS_OFFSET   
$ 08 constant DSPI_FIFO_OSC_SIZE         

$ 02F0 constant DSPI_INPUT0                
$ 02F1 constant DSPI_INPUT1                
$ 02E0 constant DSPI_OUTPUT0               
$ 02E1 constant DSPI_OUTPUT1               
$ 02E2 constant DSPI_OUTPUT2               
$ 02E3 constant DSPI_OUTPUT3               
$ 02E4 constant DSPI_OUTPUT4               
$ 02E5 constant DSPI_OUTPUT5               
$ 02E6 constant DSPI_OUTPUT6               
$ 02E7 constant DSPI_OUTPUT7         

$ 03D6 constant DSPI_INPUT_CONTROL         
$ 03D7 constant DSPI_OUTPUT_CONTROL        
$ 03DE constant DSPI_INPUT_STATUS          
$ 03DF constant DSPI_OUTPUT_STATUS   

$ 03E6 constant DSPI_CPU_INT     
$ 03EE constant DSPI_PC                         
$ 03EF constant DSPI_SIM_ADIO_FILE   \ adio control for 'C' simulator, not in hardware 
 
$ 03F6 constant DSPI_AUDLOCK               
$ 03F7 constant DSPI_CLOCK                 
$ 03FE constant DSPI_TRACE_SIM   \ trace control for 'C' simulator, not in hardware   
$ 03FF constant DSPI_NOISE     
   
$ 00010000
   dup constant DSPXB_INT_SOFT0  
2* dup constant DSPXB_INT_SOFT1
2* dup constant DSPXB_INT_SOFT2
2* dup constant DSPXB_INT_SOFT3 \ etc.
drop

DSPP_BASE $ 4018 + constant DSPX_INT_DMANEXT_ENABLE_RO
      
DSPP_BASE $ 4030 + constant DSPX_INT_UNDEROVER_SET      
DSPP_BASE $ 4034 + constant DSPX_INT_UNDEROVER_CLR      
DSPP_BASE $ 4038 + constant DSPX_INT_UNDEROVER_ENABLE   
DSPP_BASE $ 403C + constant DSPX_INT_UNDEROVER_DISABLE  

DSPI_FIFO_OSC 32 - constant DSPI_MAX_DATA_ADDRESS


DSPP_BASE $ 5200 + constant DSPX_DMA_STACK_CONTROL_CUR
DSPP_BASE $ 5208 + constant DSPX_DMA_STACK_CONTROL_NEXT
$ 10 constant DSPX_DMA_STACK_CONTROL_SIZE

$ 0001 constant DSPXB_DMA_NEXTVALID    
$ 0002 constant DSPXB_DMA_GO_FOREVER
$ 0004 constant DSPXB_INT_DMANEXT_EN   
$ 80000000 constant DSPXB_SHADOW_ADDRESS_COUNT

[then]  \ ----- ----- ----- ----- ----- ----- ----- -----
                   
\ bits for DSPP simulator trace
         1 constant DSPIB_TRACE_VERBOSE
         2 constant DSPIB_TRACE_EXEC
         4 constant DSPIB_TRACE_FDMA
         8 constant DSPIB_TRACE_RW
        16 constant DSPIB_TRACE_OUTPUT


\ Addresses for DSPP registers and memory spaces.

                     
DSPP_BASE $ 0000 + constant DSPX_CODE_MEMORY            
DSPP_BASE $ 0000 + constant DSPX_CODE_MEMORY_LOW        
DSPP_BASE $ 0FFF + constant DSPX_CODE_MEMORY_HIGH       
DSPP_BASE $ 1000 + constant DSPX_DATA_MEMORY            
DSPP_BASE $ 1000 + constant DSPX_DATA_MEMORY_LOW        
DSPP_BASE $ 1FFF + constant DSPX_DATA_MEMORY_HIGH       
DSPP_BASE $ 4000 + constant DSPX_INTERRUPT_SET          
DSPP_BASE $ 4004 + constant DSPX_INTERRUPT_CLR          
DSPP_BASE $ 4008 + constant DSPX_INTERRUPT_ENABLE       
DSPP_BASE $ 400C + constant DSPX_INTERRUPT_DISABLE   
$ 0100	constant DSPXB_INT_TIMER        
$ 0080	constant DSPXB_INT_INPUT_UNDER  
$ 0040	constant DSPXB_INT_INPUT_OVER   
$ 0020	constant DSPXB_INT_OUTPUT_UNDER 
$ 0010	constant DSPXB_INT_OUTPUT_OVER  
$ 0008	constant DSPXB_INT_UNDEROVER    
$ 0004	constant DSPXB_INT_DMALATE      
$ 0002	constant DSPXB_INT_CONSUMED     
$ 0001	constant DSPXB_INT_DMANEXT      
DSPXB_INT_DMANEXT DSPXB_INT_CONSUMED  or
	DSPXB_INT_DMALATE or
	DSPXB_INT_UNDEROVER or constant DSPXB_INT_ALL_DMA    	                               
	                                
DSPP_BASE $ 4010 + constant DSPX_INT_DMANEXT_SET        
DSPP_BASE $ 4014 + constant DSPX_INT_DMANEXT_CLR        
DSPP_BASE $ 4020 + constant DSPX_INT_CONSUMED_SET       
DSPP_BASE $ 4024 + constant DSPX_INT_CONSUMED_CLR       
DSPP_BASE $ 4028 + constant DSPX_INT_CONSUMED_ENABLE    
DSPP_BASE $ 402C + constant DSPX_INT_CONSUMED_DISABLE   
DSPP_BASE $ 5000 + constant DSPX_DMA_STACK_ADDRESS      

DSPP_BASE $ 5000 + constant  DSPX_DMA_STACK
\ STACK OFFSETS FOR EACH CHANNEL
$ 10 constant DSPX_DMA_STACK_CHANNEL_SIZE  
\ Byte OFFSETS IN STACK FOR EACH CHANNEL
$ 00 constant DSPX_DMA_ADDRESS_OFFSET                  
$ 04 constant DSPX_DMA_COUNT_OFFSET    
$ 08 constant DSPX_DMA_NEXT_ADDRESS_OFFSET
$ 0C constant DSPX_DMA_NEXT_COUNT_OFFSET

DSPP_BASE $ 6000 + constant DSPX_CHANNEL_ENABLE        
DSPP_BASE $ 6004 + constant DSPX_CHANNEL_DISABLE       
DSPP_BASE $ 6008 + constant DSPX_CHANNEL_DIRECTION_SET 
DSPP_BASE $ 600C + constant DSPX_CHANNEL_DIRECTION_CLR 
DSPP_BASE $ 6010 + constant DSPX_CHANNEL_8BIT_SET      
DSPP_BASE $ 6014 + constant DSPX_CHANNEL_8BIT_CLR      
DSPP_BASE $ 6018 + constant DSPX_CHANNEL_SQXD_SET      
DSPP_BASE $ 601C + constant DSPX_CHANNEL_SQXD_CLR      
DSPP_BASE $ 6030 + constant DSPX_CHANNEL_RESET         
DSPP_BASE $ 603C + constant DSPX_CHANNEL_STATUS        
DSPP_BASE $ 6040 + constant DSPX_AUDIO_DURATION        
DSPP_BASE $ 6044 + constant DSPX_AUDIO_TIME            
DSPP_BASE $ 6048 + constant DSPX_WAKEUP_TIME           
DSPP_BASE $ 6050 + constant AUDIO_CONFIG                 
DSPP_BASE $ 6060 + constant AUDIN_CONFIG                 
DSPP_BASE $ 6068 + constant AUDOUT_CONFIG                 
DSPP_BASE $ 6070 + constant DSPX_CONTROL
	1 constant DSPXB_GWILLING    
	2 constant DSPXB_STEP_CYCLE
	4 constant DSPXB_SNOOP
DSPP_BASE $ 6074 + constant DSPX_RESET   
	1 constant DSPXB_RESET_DSPP  
	2 constant DSPXB_RESET_AUDIN  
	4 constant DSPXB_RESET_AUDOUT  

DSPP_BASE $ 7000 + constant DSPX_TRACE_NMEM
DSPP_BASE $ 7008 + constant DSPX_TRACE_DMEMRD
DSPP_BASE $ 7010 + constant DSPX_TRACE_DMEMWR
DSPP_BASE $ 7018 + constant DSPX_TRACE_RBASE
DSPP_BASE $ 701C + constant DSPX_TRACE_ACCUME
