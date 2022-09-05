C
C  This file is part of MUMPS 5.4.1, released
C  on Tue Aug  3 09:49:43 UTC 2021
C
C
C  Copyright 1991-2021 CERFACS, CNRS, ENS Lyon, INP Toulouse, Inria,
C  Mumps Technologies, University of Bordeaux.
C
C  This version of MUMPS is provided to you free of charge. It is
C  released under the CeCILL-C license 
C  (see doc/CeCILL-C_V1-en.txt, doc/CeCILL-C_V1-fr.txt, and
C  https://cecill.info/licences/Licence_CeCILL-C_V1-en.html)
C
      SUBROUTINE DMUMPS_ALLOC_CB( INPLACE, MIN_SPACE_IN_PLACE,
     &   SSARBR, PROCESS_BANDE,
     &   MYID,N, KEEP,KEEP8,DKEEP,
     &   IW, LIW, A, LA,
     &   LRLU, IPTRLU,IWPOS,IWPOSCB,
     &   SLAVEF, PROCNODE_STEPS, DAD,
     &   PTRIST,PTRAST,STEP,PIMASTER,PAMASTER,
     &   LREQ, LREQCB, NODE_ARG, STATE_ARG, SET_HEADER,
     &   COMP, LRLUS, LRLUSM, IFLAG, IERROR )
!$    USE OMP_LIB
      USE DMUMPS_LOAD
      IMPLICIT NONE
      INTEGER N,LIW, KEEP(500)
      INTEGER(8) LA, LRLU, IPTRLU, LRLUS, LRLUSM, LREQCB
      INTEGER(8) PAMASTER(KEEP(28)), PTRAST(KEEP(28))
      INTEGER IWPOS,IWPOSCB
      INTEGER(8) :: MIN_SPACE_IN_PLACE
      INTEGER NODE_ARG, STATE_ARG
      INTEGER(8) KEEP8(150)
      DOUBLE PRECISION DKEEP(230)
      INTEGER IW(LIW),PTRIST(KEEP(28))
      INTEGER STEP(N), PIMASTER(KEEP(28))
      INTEGER, INTENT(IN) :: SLAVEF
      INTEGER, INTENT(IN) :: PROCNODE_STEPS(KEEP(28)), DAD(KEEP(28))
      INTEGER MYID, IXXP
      DOUBLE PRECISION A(LA)
      LOGICAL INPLACE, PROCESS_BANDE, SSARBR, SET_HEADER
      INTEGER COMP, LREQ, IFLAG, IERROR
      INCLUDE 'mumps_headers.h'
      INTEGER INODE_LOC,NPIV,NASS,NROW,NCB
      INTEGER ISIZEHOLE
      INTEGER(8) :: MEM_GAIN, RSIZEHOLE, LREQCB_EFF, LREQCB_WISHED
      INTEGER(8) :: DYN_SIZE, KEEP8TMPCOPY
      IF ( INPLACE ) THEN
        LREQCB_EFF = MIN_SPACE_IN_PLACE
        IF ( MIN_SPACE_IN_PLACE > 0_8 ) THEN
          LREQCB_WISHED = LREQCB
        ELSE
          LREQCB_WISHED = 0_8
        ENDIF
      ELSE
        LREQCB_EFF = LREQCB
        LREQCB_WISHED = LREQCB
      ENDIF
      IF (IWPOSCB.EQ.LIW) THEN
        IF (LREQ.NE.KEEP(IXSZ).OR.LREQCB.NE.0_8
     &      .OR. .NOT. SET_HEADER) THEN
          WRITE(*,*) "Internal error in DMUMPS_ALLOC_CB ",
     &      SET_HEADER, LREQ, LREQCB
          CALL MUMPS_ABORT()
        ENDIF
        IF (IWPOSCB-IWPOS+1 .LT. KEEP(IXSZ)) THEN
          WRITE(*,*) "Problem with integer stack size",IWPOSCB,
     &               IWPOS, KEEP(IXSZ)
          IFLAG  = -8
          IERROR = LREQ
          RETURN
        ENDIF
        IWPOSCB=IWPOSCB-KEEP(IXSZ)
        IW(IWPOSCB+1+XXI)=KEEP(IXSZ)
        CALL MUMPS_STOREI8(0_8,IW(IWPOSCB+1+XXR))
        CALL MUMPS_STOREI8(0_8,IW(IWPOSCB+1+XXD))
        IW(IWPOSCB+1+XXN)=-919191
        IW(IWPOSCB+1+XXS)=S_NOTFREE
        IW(IWPOSCB+1+XXP)=TOP_OF_STACK
        RETURN
      ENDIF
      CALL MUMPS_GETI8( DYN_SIZE, IW(IWPOSCB+1 + XXD))
      IF (DYN_SIZE .EQ. 0_8
     &    .AND. KEEP(214).EQ.1.AND.
     &    KEEP(216).EQ.1.AND.
     &    IWPOSCB.NE.LIW) THEN
       IF (IW(IWPOSCB+1 + XXS).EQ.S_NOLCBNOCONTIG.OR.
     &     IW(IWPOSCB+1 + XXS).EQ.S_NOLCBNOCONTIG38) THEN
        NCB  = IW( IWPOSCB+1 + KEEP(IXSZ) )
        NROW = IW( IWPOSCB+1 + KEEP(IXSZ) + 2)
        NPIV = IW( IWPOSCB+1 + KEEP(IXSZ) + 3)
        INODE_LOC= IW( IWPOSCB+1 + XXN)
        CALL DMUMPS_GET_SIZEHOLE(IWPOSCB+1,IW,LIW,
     &                          ISIZEHOLE,RSIZEHOLE)
        IF (IW(IWPOSCB+1 + XXS).EQ.S_NOLCBNOCONTIG) THEN
          CALL DMUMPS_MAKECBCONTIG(A,LA,IPTRLU+1_8,
     &                           NROW,NCB,NPIV+NCB,0,
     &                           IW(IWPOSCB+1 + XXS),RSIZEHOLE)
          IW(IWPOSCB+1 + XXS) =S_NOLCLEANED
          MEM_GAIN            = int(NROW,8)*int(NPIV,8)
        ENDIF
        IF (IW(IWPOSCB+1 + XXS).EQ.S_NOLCBNOCONTIG38) THEN
          NASS = IW( IWPOSCB+1 + KEEP(IXSZ) + 4)
          CALL DMUMPS_MAKECBCONTIG(A,LA,IPTRLU+1_8,
     &                           NROW,NCB,NPIV+NCB,NASS-NPIV,
     &                           IW(IWPOSCB+1 + XXS),RSIZEHOLE)
          IW(IWPOSCB+1 + XXS) =S_NOLCLEANED38
          MEM_GAIN = int(NROW,8)*int(NPIV+NCB-(NASS-NPIV),8)
        ENDIF
        IF (ISIZEHOLE.NE.0) THEN
          CALL DMUMPS_ISHIFT( IW,LIW,IWPOSCB+1,
     &                       IWPOSCB+IW(IWPOSCB+1+XXI),
     &                       ISIZEHOLE )
          IWPOSCB=IWPOSCB+ISIZEHOLE
          IW(IWPOSCB+1+XXP+IW(IWPOSCB+1+XXI))=IWPOSCB+1
          PTRIST(STEP(INODE_LOC))=PTRIST(STEP(INODE_LOC))+
     &    ISIZEHOLE
        ENDIF
        CALL MUMPS_SUBTRI8TOARRAY(IW(IWPOSCB+1+XXR), MEM_GAIN)
        IPTRLU              = IPTRLU+MEM_GAIN+RSIZEHOLE
        LRLU                = LRLU+MEM_GAIN+RSIZEHOLE
        PTRAST(STEP(INODE_LOC))=
     &  PTRAST(STEP(INODE_LOC))+MEM_GAIN+RSIZEHOLE
       ENDIF
      ENDIF
      IF (LRLU.LT.LREQCB_WISHED)THEN
         IF (LREQCB_EFF.LT.LREQCB_WISHED) THEN
          CALL DMUMPS_COMPRE_NEW(N,KEEP(28),IW,LIW,A,LA,
     &                    LRLU,IPTRLU,IWPOS,IWPOSCB,
     &                    PTRIST,PTRAST,
     &                    STEP, PIMASTER,PAMASTER,KEEP(216),LRLUS,
     &                    KEEP(IXSZ), COMP, DKEEP(97), MYID,
     &                    SLAVEF, KEEP(199), PROCNODE_STEPS, DAD)
         ENDIF
      ENDIF
      CALL DMUMPS_GET_SIZE_NEEDED 
     &                   (LREQ, LREQCB_EFF, .FALSE.,
     &                    KEEP(1), KEEP8(1),
     &                    N,KEEP(28),IW,LIW,A,LA,
     &                    LRLU,IPTRLU,IWPOS,IWPOSCB,
     &                    PTRIST,PTRAST,
     &                    STEP, PIMASTER,PAMASTER,KEEP(216),LRLUS,
     &                    KEEP(IXSZ), COMP, DKEEP(97), MYID,
     &                    SLAVEF, PROCNODE_STEPS, DAD, 
     &                    IFLAG, IERROR)
      IF (IFLAG.LT.0) GOTO 650
      IXXP=IWPOSCB+XXP+1
      IF (IXXP.GT.LIW) THEN
        WRITE(*,*) "Internal error 3 in DMUMPS_ALLOC_CB ",IXXP
      ENDIF
      IF (IW(IXXP).GT.0) THEN
        WRITE(*,*) "Internal error 2 in DMUMPS_ALLOC_CB ",IW(IXXP),IXXP
      ENDIF
      IWPOSCB = IWPOSCB - LREQ
      IF (SET_HEADER) THEN
        IW(IXXP)= IWPOSCB + 1
        IW(IWPOSCB+1:IWPOSCB+1+KEEP(IXSZ))=-99999 
        IW(IWPOSCB+1+XXI)=LREQ
        CALL MUMPS_STOREI8(LREQCB, IW(IWPOSCB+1+XXR))
        CALL MUMPS_STOREI8(0_8, IW(IWPOSCB+1+XXD))
        IW(IWPOSCB+1+XXS)=STATE_ARG
        IW(IWPOSCB+1+XXN)=NODE_ARG
        IW(IWPOSCB+1+XXP)=TOP_OF_STACK
        IW(IWPOSCB+1+XXNBPR)=0 
      ENDIF
      IPTRLU = IPTRLU - LREQCB
      LRLU   = LRLU - LREQCB
      LRLUS  = LRLUS - LREQCB_EFF
      LRLUSM = min(LRLUS, LRLUSM)
      IF (KEEP(405) .EQ. 0) THEN
        KEEP8(69) = KEEP8(69) + LREQCB_EFF
        KEEP8(68) = max(KEEP8(69), KEEP8(68))
      ELSE
!$OMP   ATOMIC CAPTURE
        KEEP8(69) = KEEP8(69) + LREQCB_EFF
        KEEP8TMPCOPY = KEEP8(69)
!$OMP   END ATOMIC
!$OMP   ATOMIC UPDATE
        KEEP8(68) = max(KEEP8TMPCOPY, KEEP8(68))
!$OMP   END ATOMIC
      ENDIF
      CALL DMUMPS_LOAD_MEM_UPDATE(SSARBR,PROCESS_BANDE,
     &              LA-LRLUS,0_8,LREQCB_EFF,KEEP,KEEP8,LRLUS)
 650  CONTINUE
      RETURN
      END SUBROUTINE DMUMPS_ALLOC_CB