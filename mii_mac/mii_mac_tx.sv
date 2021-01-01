`default_nettype none

module mii_mac_tx #(
    parameter PREAMBLE_CHARACTER = 8'h55,
    parameter SFD_CHARACTER = 8'hd5
) (
    input wire clock,
    input wire aresetn,
    
    output reg [3:0] mii_d,
    output reg       mii_en,
    output reg       mii_er,

    input  wire [7:0] saxis_tdata,
    input  wire        saxis_tvalid,
    output wire        saxis_tready,
    input  wire        saxis_tuser,
    input  wire        saxis_tlast
);

logic [7:0] append_crc_out_tdata;
logic       append_crc_out_tvalid;
logic       append_crc_out_tready;
logic       append_crc_out_tuser = 0;
logic       append_crc_out_tlast;

append_crc append_crc_inst (
    .clock(clock),
    .aresetn(aresetn),

    .saxis_tdata(saxis_tdata),
    .saxis_tvalid(saxis_tvalid),
    .saxis_tready(saxis_tready),
    .saxis_tuser(saxis_tuser),
    .saxis_tlast(saxis_tlast),

    .maxis_tdata(append_crc_out_tdata),
    .maxis_tvalid(append_crc_out_tvalid),
    .maxis_tready(append_crc_out_tready),
    .maxis_tuser(append_crc_out_tuser),
    .maxis_tlast(append_crc_out_tlast)
);

logic [7:0] prepend_preamble_out_tdata;
logic       prepend_preamble_out_tvalid;
logic       prepend_preamble_out_tready;
logic       prepend_preamble_out_tuser = 0;
logic       prepend_preamble_out_tlast;

prepend_preamble #(
    .PREAMBLE(PREAMBLE_CHARACTER), .SFD(SFD_CHARACTER)
) prepend_preamble_inst (
    .clock(clock),
    .aresetn(aresetn),

    .saxis_tdata(append_crc_out_tdata),
    .saxis_tvalid(append_crc_out_tvalid),
    .saxis_tready(append_crc_out_tready),
    .saxis_tlast(append_crc_out_tlast),

    .maxis_tdata(prepend_preamble_out_tdata),
    .maxis_tvalid(prepend_preamble_out_tvalid),
    .maxis_tready(prepend_preamble_out_tready),
    .maxis_tlast(prepend_preamble_out_tlast)
);

axis_to_mii axis_to_mii_inst (
    .clock(clock),
    .aresetn(aresetn),

    .saxis_tdata(prepend_preamble_out_tdata),
    .saxis_tvalid(prepend_preamble_out_tvalid),
    .saxis_tready(prepend_preamble_out_tready),
    .saxis_tlast(prepend_preamble_out_tlast),

    .mii_d(mii_d),
    .mii_en(mii_en),
    .mii_er(mii_er)
);

endmodule

`default_nettype wire
