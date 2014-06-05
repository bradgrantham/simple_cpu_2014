        // simple multiply; load r0 and r1 and then mult r0 by r1
        moviu r0, 0xdead
        addi r0, 0xbeef
        moviu r1, 0xcafe
        addi r1, 0xbabe
        mult r0, r1
        hlt
