`default_nettype none

module rmii_to_axis (
    input wire clock,
    input wire aresetn,
    
    input wire [2:0] rmii_d,
    input wire       rmii_dv,

    output logic  [7:0]  maxis_tdata,
    output logic         maxis_tvalid,
    output logic         maxis_tuser,
    output logic         maxis_tlast
);

localparam SFD = 8'hd5;

logic sfd_detected;

logic [2:0] phase = 0;

logic prev_in_frame = 0;
logic in_frame = 0;

logic [2:0] dv_buf_3;   // DV input buffer for 3 cycles.
logic [5:0] d_buf_3;    // D  input buffer for 3 cycles.

always_ff @(posedge clock) begin
    if( !aresetn ) begin
        dv_buf_3 <= 0;
        d_buf_3 <= 0;
    end
    else begin
        dv_buf_3 <= {rmii_dv, dv_buf_3[2:1]};
        d_buf_3  <= {rmii_d,  d_buf_3[5:2]};
    end
end 

logic [3:0] dv_4;   // current and prev 3 cycles DV signal.
logic [7:0] d_4;    // current and prev 3 cycles DV signal.

always_comb begin
    dv_4 = {rmii_dv, dv_buf_3};
    d_4 =  {rmii_d,  d_buf_3};
end

assign sfd_detected = dv_4 == 4'b1111 && d_4 == SFD && !in_frame;

always_ff @(posedge clock) begin
    if( !aresetn ) begin
        phase <= 0;
        in_frame <= 0;
        prev_in_frame <= 0;
    end
    else begin
        prev_in_frame <= in_frame;
        phase <= (sfd_detected || phase == 2'd3) ? 0 : phase + 1;
        in_frame <=   sfd_detected ? 1
                    : !rmii_dv ? 0
                    : in_frame;
    end
end

always_ff @(posedge clock) begin
    if( !aresetn ) begin
        maxis_tdata <= 0;
        maxis_tvalid <= 0;
        maxis_tlast <= 0;
        maxis_tuser <= 0;
    end
    else begin
        maxis_tvalid <= 0;
        
        // Update maxis_tdata
        if( dv_4 == 4'b1111 && in_frame && phase == 2'd3 ) begin
            maxis_tdata <= d_4; // Hold data
        end
        if( prev_in_frame && in_frame && phase == 2'd0 ) begin
            maxis_tvalid <= 1;
            maxis_tlast <= !rmii_dv;    // DV signal is deasserted. 
        end
    end
end


endmodule

`default_nettype wire
