        // load up r0 and halt.
        moviu r0, 0xdead
        addi r0, 0xbeef
        hlt
