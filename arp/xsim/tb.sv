`timescale 1ns/1ps

module tb();
    logic clock;
    logic aresetn;

    axis_if #(.DATA_WIDTH(1)) tb_maxis_if(.clock(clock), .aresetn(aresetn));
    axis_if #(.DATA_WIDTH(1)) tb_saxis_if(.clock(clock), .aresetn(aresetn));

    typedef struct packed {
        bit [31:0] padding1;
        bit [31:0] ip_addr;
        bit [15:0] padding0;
        bit [47:0] hw_addr;
    } config_t;

    config_t default_config = '{padding1: 0, ip_addr: 32'hc0a80102, padding0: 0, hw_addr: 48'haabbccddeeff };

    ethernet_service dut(
        .ap_clk(clock),
        .ap_rst_n(aresetn),
        .config_r(default_config),
        .in_r_TDATA(tb_maxis_if.tdata),
        .in_r_TVALID(tb_maxis_if.tvalid),
        .in_r_TREADY(tb_maxis_if.tready),
        .in_r_TKEEP(tb_maxis_if.tkeep),
        .in_r_TSTRB(tb_maxis_if.tkeep),
        .in_r_TLAST(tb_maxis_if.tlast),
        .out_r_TDATA(tb_saxis_if.tdata),
        .out_r_TVALID(tb_saxis_if.tvalid),
        .out_r_TREADY(tb_saxis_if.tready),
        .out_r_TKEEP(tb_saxis_if.tkeep),
        .out_r_TSTRB(tb_saxis_if.tkeep),
        .out_r_TLAST(tb_saxis_if.tlast)
    );
    
    initial begin
        clock = 0;
    end 
    always #(5) begin
        clock = ~clock;
    end
    
    typedef byte buffer_t[];
    function automatic buffer_t readall(string path);
    begin
        int fd;
        int length;
        byte buffer[1024];

        fd = $fopen(path, "r");
        void'($fseek(fd, 0, 2));
        length = $ftell(fd);
        void'($fseek(fd, 0, 0));
        readall = new[length];
        for(int i = 0; i < readall.size(); ) begin
            int bytes_read;
            int bytes_to_read;
            bytes_to_read = length - i < 1024 ? length - i : 1024;
            bytes_read = $fread(buffer, fd, 0, bytes_to_read);
            for(int j = 0; j < bytes_read; j++ ) readall[i + j] = buffer[j];
            i += bytes_read;
        end
    end
    endfunction

    module stimuli (
        input logic clock,
        output logic aresetn,
        axis_if.master tb_maxis,
        axis_if.slave tb_saxis
    );
        initial begin
            byte input_data[];
            byte expected_data[];

            input_data = readall("../data/arp.input.bin");
            //foreach(input_data[i]) $display("%02h ", input_data[i]);
            expected_data = readall("../data/arp.expected.bin");
            //foreach(expected_data[i]) $display("%02h ", expected_data[i]);

            aresetn <= 0;
            tb_maxis.master_init;
            tb_saxis.slave_init;
            repeat(4) @(posedge clock);

            aresetn <= 1;
            @(posedge clock);
            #1;
            
            fork
                fork
                    begin
                        for(int i = 0; i < input_data.size(); i++ ) begin
                            tb_maxis.master_send(input_data[i], '1, i == input_data.size() - 1, 0);
                            //repeat($urandom_range(0, 2)) @(posedge clock);
                        end
                    end
                    begin
                        for(int i = 0; i < expected_data.size(); i++ ) begin
                            bit [7:0]  tdata;
                            bit [0:0]  tkeep;
                            bit        tuser;
                            bit        tlast;
                            bit [7:0]  tdata_expected;
                            bit        tlast_expected;

                            tdata_expected = expected_data[i];
                            tlast_expected = i == expected_data.size() - 1;

                            tb_saxis.slave_receive(tdata, tkeep, tlast, tuser, 32'h7fffffff);
                            if( tdata_expected != tdata ) $error("#%02d tdata mismatch, expected=%016h, actual=%016h", i, tdata_expected, tdata);
                            if( tlast_expected != tlast ) $error("#%02d tlast mismatch, expected=%d, actual=%d"      , i, tlast_expected, tlast);
                        end
                    end
                join
                begin
                    repeat(expected_data.size() + 1000) @(posedge clock); $error("Timed out");
                end
            join_any
            
            $finish;
        end
    endmodule
    stimuli stimuli_inst (
        .tb_maxis(tb_maxis_if),
        .tb_saxis(tb_saxis_if),
        .*
    );
endmodule
