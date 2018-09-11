`define NO_TIMEOUT

initial
   begin
      $display(" ===============================================");
      $display("|                 START SIMULATION              |");
      $display(" ===============================================");
      repeat(5) @(posedge mclk);
      stimulus_done = 0;

      // simulate keypad input '123A'
      p2_din[7:0] = 8'hEF;

      /* ----------------------  END OF TEST --------------- */
      @(posedge r2[4]);

      stimulus_done = 1;
      $display(" ===============================================");
      $display("|               SIMULATION DONE                 |");
      $display("|       (stopped through verilog stimulus)      |");
      $display(" ===============================================");
      $finish;
   end

always @(posedge mclk)
    if (irq_detect) $display("[stimulus] IRQ after instruction 0x%h: %s", current_inst_pc, inst_full);
