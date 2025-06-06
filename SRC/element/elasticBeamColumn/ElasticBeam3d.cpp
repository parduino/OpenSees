/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision$
// $Date$
// $URL$
                                                                        
                                                                        
// File: ~/model/ElasticBeam3d.C
//
// Written: fmk 11/95
// Revised:
//
// Purpose: This file contains the class definition for ElasticBeam3d.
// ElasticBeam3d is a 3d beam element. As such it can only
// connect to a node with 6-dof. 

#include <ElasticBeam3d.h>
#include <Domain.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>

#include <CrdTransf.h>
#include <Damping.h>
#include <Information.h>
#include <Parameter.h>
#include <ElementResponse.h>
#include <ElementalLoad.h>
#include <Renderer.h>
#include <SectionForceDeformation.h>
#include <ID.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <elementAPI.h>

Matrix ElasticBeam3d::K(12,12);
Vector ElasticBeam3d::P(12);
Matrix ElasticBeam3d::kb(6,6);

void* OPS_ElasticBeam3d(void)
{
    int numArgs = OPS_GetNumRemainingInputArgs();

    // Read the optional arguments first
    double mass = 0.0;
    int dampingTag = 0;
    Damping *theDamping = 0;
    int cMass = 0;
    int releasez = 0;
    int releasey = 0;
    int numData = 1;
    int numOptionalArgs = 0;
    while(OPS_GetNumRemainingInputArgs() > 0) {
	std::string theType = OPS_GetString();
	if (theType == "-mass") {
	  numOptionalArgs++;
	  if(OPS_GetNumRemainingInputArgs() > 0) {
	    numData = 1;	    
	    if(OPS_GetDoubleInput(&numData,&mass) < 0)
	      return 0;
	    numOptionalArgs++;	    
	  }
	} else if (theType == "-cMass") {
	  numOptionalArgs++;
	  cMass = 1;
	} else if (theType == "-releasez") {
	  numOptionalArgs++;	  
	  if (OPS_GetNumRemainingInputArgs() > 0) {
	    numData = 1;	    
	    if (OPS_GetIntInput(&numData, &releasez) < 0) {
	      opserr << "WARNING: failed to get releasez";
	      return 0;
	    }
	    numOptionalArgs++;	    
	  }
	} else if (theType == "-releasey") {
	  numOptionalArgs++;	  
	  if (OPS_GetNumRemainingInputArgs() > 0) {
	    numData = 1;
	    if (OPS_GetIntInput(&numData, &releasey) < 0) {
	      opserr << "WARNING: failed to get releasey";
	      return 0;
	    }
	    numOptionalArgs++;
	  }
	} else if(theType == "-damp"){
	  numOptionalArgs++;	  
	  if(OPS_GetNumRemainingInputArgs() > 0) {
	    numData = 1;
	    if(OPS_GetIntInput(&numData,&dampingTag) < 0) return 0;
	    theDamping = OPS_getDamping(dampingTag);
	    if(theDamping == 0) {
	      opserr<<"damping not found\n";
	      return 0;
	    }
	  }
	} 
    }

    if (numArgs > 0) {
      OPS_ResetCurrentInputArg(-numArgs);    
    }
    numArgs = numArgs - numOptionalArgs;
      
    if(numArgs < 10 && numArgs != 5) {
	opserr<<"insufficient arguments:eleTag,iNode,jNode,<A,E,G,J,Iy,Iz>or<sectionTag>,transfTag\n";
	return 0;
    }

    int ndm = OPS_GetNDM();
    int ndf = OPS_GetNDF();
    if(ndm != 3 || ndf != 6) {
	opserr<<"ndm must be 3 and ndf must be 6\n";
	return 0;
    }

    // inputs: 
    int iData[3];
    numData = 3;
    if(OPS_GetIntInput(&numData,&iData[0]) < 0) return 0;

    SectionForceDeformation* theSection = 0;
    CrdTransf* theTrans = 0;
    double data[6];
    int transfTag, secTag;
    
    if(numArgs == 5) {
	numData = 1;
	if(OPS_GetIntInput(&numData,&secTag) < 0) return 0;
	if(OPS_GetIntInput(&numData,&transfTag) < 0) return 0;

	theSection = OPS_getSectionForceDeformation(secTag);
	if(theSection == 0) {
	    opserr<<"no section is found\n";
	    return 0;
	}
	theTrans = OPS_getCrdTransf(transfTag);
	if(theTrans == 0) {
	    opserr<<"no CrdTransf is found\n";
	    return 0;
	}
    } else {
	numData = 6;
	if(OPS_GetDoubleInput(&numData,&data[0]) < 0) return 0;
	numData = 1;
	if(OPS_GetIntInput(&numData,&transfTag) < 0) return 0;
	theTrans = OPS_getCrdTransf(transfTag);
	if(theTrans == 0) {
	    opserr<<"no CrdTransf is found\n";
	    return 0;
	}
    }
    
    if (theSection != 0) {
      return new ElasticBeam3d(iData[0],iData[1],iData[2],*theSection,*theTrans,mass,cMass,releasez, releasey,theDamping); 
    } else {
	return new ElasticBeam3d(iData[0],data[0],data[1],data[2],data[3],data[4],
				 data[5],iData[1],iData[2],*theTrans, mass,cMass,releasez,releasey,theDamping);
    }
}

void *OPS_ElasticBeam3d(const ID &info) {
    // 1. data needed
    int iData[3];
    double data[6];
    int transfTag;
    double mass = 0.0;
    int cMass = 0;
    int releasez = 0;
    int releasey = 0;
    int numData = 1;

    int ndm = OPS_GetNDM();
    int ndf = OPS_GetNDF();
    if (ndm != 3 || ndf != 6) {
        opserr << "ndm must be 3 and ndf must be 6\n";
        return 0;
    }

    // 2. regular elements
    if (info.Size() == 0) {
        if (OPS_GetNumRemainingInputArgs() < 3) {
            opserr << "insufficient "
                      "arguments:eleTag,iNode,jNode\n";
            return 0;
        }

        // inputs:
        int numData = 3;
        if (OPS_GetIntInput(&numData, &iData[0]) < 0) {
            opserr << "WARNING failed to read integers\n";
            return 0;
        }
    }

    // 3. regular elements or save data
    if (info.Size() == 0 || info(0) == 1) {
        if (OPS_GetNumRemainingInputArgs() < 6) {
            opserr << "insufficient "
                      "arguments:A, E, G, J, Iy, Iz\n";
            return 0;
        }

        // Read A, E, G, J, Iy, Iz
        numData = 6;
        if (OPS_GetDoubleInput(&numData, &data[0]) < 0) {
            opserr << "WARNING failed to read doubles\n";
            return 0;
        }

        if (OPS_GetNumRemainingInputArgs() < 1) {
            opserr << "WARNING: transfTag is needed\n";
        }
        numData = 1;
        if (OPS_GetIntInput(&numData, &transfTag) < 0) {
            opserr << "WARNING transfTag is not integer\n";
            return 0;
        }

        while (OPS_GetNumRemainingInputArgs() > 0) {
            std::string theType = OPS_GetString();
            if (theType == "-mass") {
                if (OPS_GetNumRemainingInputArgs() > 0) {
                    if (OPS_GetDoubleInput(&numData, &mass) < 0) {
                        opserr << "WARNING: failed to read mass\n";
                        return 0;
                    }
                }
            } else if (theType == "-cMass") {
                cMass = 1;
            } else if (theType == "-releasez") {
                if (OPS_GetNumRemainingInputArgs() > 0) {
                    if (OPS_GetIntInput(&numData, &releasez) < 0) {
                        opserr << "WARNING: failed to get releasez";
                        return 0;
                    }
                }
            } else if (theType == "-releasey") {
                if (OPS_GetNumRemainingInputArgs() > 0) {
                    if (OPS_GetIntInput(&numData, &releasey) < 0) {
                        opserr << "WARNING: failed to get releasey";
                        return 0;
                    }
                }
            }
        }
    }

    // 4. save data
    static std::map<int, Vector> meshdata;
    if (info.Size() > 0 && info(0) == 1) {
        if (info.Size() < 2) {
            opserr << "WARNING: need info -- inmesh, meshtag\n";
        }

        Vector &mdata = meshdata[info(1)];
        mdata.resize(11);
        mdata(0) = data[0];
        mdata(1) = data[1];
        mdata(2) = data[2];
        mdata(3) = data[3];
        mdata(4) = data[4];
        mdata(5) = data[5];
        mdata(6) = mass;
        mdata(7) = cMass;
        mdata(8) = releasez;
        mdata(9) = releasey;
        mdata(10) = transfTag;
        return &meshdata;
    }

    // 5: load data
    if (info.Size() > 0 && info(0) == 2) {
        if (info.Size() < 5) {
            opserr << "WARNING: need info -- inmesh, meshtag, "
                      "eleTag, nd1, nd2\n";
            return 0;
        }

        Vector &mdata = meshdata[info(1)];
        if (mdata.Size() < 11) return 0;
        data[0] = mdata(0);
        data[1] = mdata(1);
        data[2] = mdata(2);
        data[3] = mdata(3);
        data[4] = mdata(4);
        data[5] = mdata(5);
        mass = mdata(6);
        cMass = (int)mdata(7);
        releasez = (int)mdata(8);
        releasey = (int)mdata(9);
        transfTag = (int)mdata(10);

        iData[0] = info(2);
        iData[1] = info(3);
        iData[2] = info(4);
    }

    // 6: create element
    CrdTransf *theTrans = OPS_getCrdTransf(transfTag);
    if (theTrans == 0) {
        opserr << "no CrdTransf is found\n";
        return 0;
    }

    return new ElasticBeam3d(iData[0], data[0], data[1], data[2],
                             data[3], data[4], data[5], iData[1],
                             iData[2], *theTrans, mass, cMass,
                             releasez, releasey);
}

ElasticBeam3d::ElasticBeam3d()
  :Element(0,ELE_TAG_ElasticBeam3d), 
   A(0.0), E(0.0), G(0.0), Jx(0.0), Iy(0.0), Iz(0.0), rho(0.0), cMass(0),
   releasez(0), releasey(0),
   Q(12), q(6), wx(0.0), wy(0.0), wz(0.0),
   connectedExternalNodes(2), theCoordTransf(0),
   theDamping(0)
{
  // does nothing
  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;
  q0[3] = 0.0;
  q0[4] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;
  p0[3] = 0.0;
  p0[4] = 0.0;
  
  // set node pointers to NULL
  for (int i=0; i<2; i++)
    theNodes[i] = 0;      
}

ElasticBeam3d::ElasticBeam3d(int tag, double a, double e, double g, 
			     double jx, double iy, double iz, int Nd1, int Nd2, 
			     CrdTransf &coordTransf, double r, int cm, int relz, int rely,
			     Damping *damping)
  :Element(tag,ELE_TAG_ElasticBeam3d), 
   A(a), E(e), G(g), Jx(jx), Iy(iy), Iz(iz), rho(r), cMass(cm),
   releasez(relz), releasey(rely),
   Q(12), q(6), wx(0.0), wy(0.0), wz(0.0),
   connectedExternalNodes(2), theCoordTransf(0), theDamping(0)
{
  connectedExternalNodes(0) = Nd1;
  connectedExternalNodes(1) = Nd2;
  
  theCoordTransf = coordTransf.getCopy3d();
  
  if (!theCoordTransf) {
    opserr << "ElasticBeam3d::ElasticBeam3d -- failed to get copy of coordinate transformation\n";
    exit(-1);
  }

  // Make no release if input not 0, 1, 2, or 3
  if (releasez < 0 || releasez > 3)
    releasez = 0;
  if (releasey < 0 || releasey > 3)
    releasey = 0;  
  
  if (damping)
  {
    theDamping =(*damping).getCopy();
    
    if (!theDamping) {
      opserr << "ElasticBeam3d::ElasticBeam3d -- failed to get copy of damping\n";
      //exit(-1); // this is not a fatal error...
      theDamping = 0;
    }
  }

  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;
  q0[3] = 0.0;
  q0[4] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;
  p0[3] = 0.0;
  p0[4] = 0.0;

  // set node pointers to NULL
  for (int i=0; i<2; i++)
    theNodes[i] = 0;      
}

ElasticBeam3d::ElasticBeam3d(int tag, int Nd1, int Nd2, SectionForceDeformation &section,  
			     CrdTransf &coordTransf, double r, int cm, int relz, int rely,
			     Damping *damping)
  :Element(tag,ELE_TAG_ElasticBeam3d), 
      A(0.0), E(1.0), G(1.0), Jx(0.0), Iy(0.0), Iz(0.0),
   rho(r), cMass(cm), releasez(relz), releasey(rely),
   Q(12), q(6), wx(0.0), wy(0.0), wz(0.0),
   connectedExternalNodes(2), theCoordTransf(0), theDamping(0)
{
  // Try to find E in the section
  const char *argv[1] = {"E"};
  int argc = 1;
  Parameter param;
  int ok = section.setParameter(argv, argc, param);
  if (ok >= 0)
    E = param.getValue();

  if (E == 0.0) {
    opserr << "ElasticBeam3d::ElasticBeam3d - E from section is zero, using E = 1" << endln;
    E = 1.0;
  }

  // Try to find G in the section
  argv[0] = {"G"};
  ok = section.setParameter(argv, argc, param);
  if (ok >= 0)
    G = param.getValue();

  if (G == 0.0) {
    opserr << "ElasticBeam3d::ElasticBeam3d - G from section is zero, using G = 1" << endln;
    G = 1.0;
  }  
  
  const Matrix &sectTangent = section.getInitialTangent();
  const ID &sectCode = section.getType();
  for (int i=0; i<sectCode.Size(); i++) {
    int code = sectCode(i);
    switch(code) {
    case SECTION_RESPONSE_P:
      A = sectTangent(i,i)/E;
      break;
    case SECTION_RESPONSE_MZ:
      Iz = sectTangent(i,i)/E;
      break;
    case SECTION_RESPONSE_MY:
      Iy = sectTangent(i,i)/E;
      break;
    case SECTION_RESPONSE_T:
      Jx = sectTangent(i,i)/G;
      break;
    default:
      break;
    }
  }
  
  if (Jx == 0.0) {
    opserr << "ElasticBeam3d::ElasticBeam3d -- no torsion in section -- continuing with GJ = 0\n";
    //Jx = 1.0e10;
  }

  connectedExternalNodes(0) = Nd1;
  connectedExternalNodes(1) = Nd2;
  
  theCoordTransf = coordTransf.getCopy3d();
  
  if (!theCoordTransf) {
    opserr << "ElasticBeam3d::ElasticBeam3d -- failed to get copy of coordinate transformation\n";
    exit(-1);
  }

  // Make no release if input not 0, 1, 2, or 3
  if (releasez < 0 || releasez > 3)
    releasez = 0;
  if (releasey < 0 || releasey > 3)
    releasey = 0;
  
  if (damping)
  {
    theDamping =(*damping).getCopy();
    
    if (!theDamping) {
      opserr << "ElasticBeam3d::ElasticBeam3d -- failed to get copy of damping\n";
      //exit(-1); // Not a fatal error...
      theDamping = 0;
    }
  }

  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;
  q0[3] = 0.0;
  q0[4] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;
  p0[3] = 0.0;
  p0[4] = 0.0;

  // set node pointers to NULL
  for (int i=0; i<2; i++)
    theNodes[i] = 0;      
}

ElasticBeam3d::~ElasticBeam3d()
{
  if (theCoordTransf)
    delete theCoordTransf;
  if (theDamping) delete theDamping;
}

int
ElasticBeam3d::getNumExternalNodes(void) const
{
    return 2;
}

const ID &
ElasticBeam3d::getExternalNodes(void) 
{
    return connectedExternalNodes;
}

Node **
ElasticBeam3d::getNodePtrs(void) 
{
  return theNodes;
}

int
ElasticBeam3d::getNumDOF(void)
{
    return 12;
}

void
ElasticBeam3d::setDomain(Domain *theDomain)
{
  if (theDomain == 0) {
    opserr << "ElasticBeam3d::setDomain -- Domain is null\n";
    exit(-1);
  }
    
    theNodes[0] = theDomain->getNode(connectedExternalNodes(0));
    theNodes[1] = theDomain->getNode(connectedExternalNodes(1));    
    

    if (theNodes[0] == 0) {
      opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Node 1: " << connectedExternalNodes(0) << " does not exist\n";
      exit(-1);
    }
			      
    if (theNodes[1] == 0) {
      opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Node 2: " << connectedExternalNodes(1) << " does not exist\n";
      exit(-1);
    }

    int dofNd1 = theNodes[0]->getNumberDOF();
    int dofNd2 = theNodes[1]->getNumberDOF();    
    
    if (dofNd1 != 6) {
      opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Node 1: " << connectedExternalNodes(0) 
	     << " has incorrect number of DOF\n";
      exit(-1);
    }
    
    if (dofNd2 != 6) {
      opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Node 2: " << connectedExternalNodes(1) 
	     << " has incorrect number of DOF\n";
      exit(-1);
    }
	
    this->DomainComponent::setDomain(theDomain);
    
    if (theCoordTransf->initialize(theNodes[0], theNodes[1]) != 0) {
	opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Error initializing coordinate transformation\n";
	exit(-1);
    }
    
    if (theDamping && theDamping->setDomain(theDomain, 6)) {
	opserr << "ElasticBeam3d::setDomain -- Error initializing damping\n";
	exit(-1);
    }

    double L = theCoordTransf->getInitialLength();

    if (L == 0.0) {
      opserr << "ElasticBeam3d::setDomain  tag: " << this->getTag() << " -- Element has zero length\n";
      exit(-1);
    }
}

int
ElasticBeam3d::setDamping(Domain *theDomain, Damping *damping)
{
  if (theDomain && damping)
  {
    if (theDamping) delete theDamping;

    theDamping =(*damping).getCopy();
    
    if (!theDamping) {
      opserr << "ElasticBeam3d::setDamping -- failed to get copy of damping\n";
      return -1;
    }
    if (theDamping->setDomain(theDomain, 6)) {
      opserr << "ElasticBeam3d::setDamping -- Error initializing damping\n";
      return -2;
    }
  }
  
  return 0;
}

int
ElasticBeam3d::commitState()
{
  int retVal = 0;
  // call element commitState to do any base class stuff
  if ((retVal = this->Element::commitState()) != 0) {
    opserr << "ElasticBeam3d::commitState () - failed in base class";
  }    
  retVal += theCoordTransf->commitState();
  if (theDamping) retVal += theDamping->commitState();
  return retVal;
}

int
ElasticBeam3d::revertToLastCommit()
{
  int retVal = 0;
  retVal += theCoordTransf->revertToLastCommit();
  if (theDamping) retVal += theDamping->revertToLastCommit();
  return retVal;
}

int
ElasticBeam3d::revertToStart()
{
  int retVal = 0;
  retVal += theCoordTransf->revertToStart();
  if (theDamping) retVal += theDamping->revertToStart();
  return retVal;
}

int
ElasticBeam3d::update(void)
{
  return theCoordTransf->update();
}

const Matrix &
ElasticBeam3d::getTangentStiff(void)
{
  const Vector &v = theCoordTransf->getBasicTrialDisp();
  
  double L = theCoordTransf->getInitialLength();
  double oneOverL = 1.0/L;
  double EoverL   = E*oneOverL;
  double EAoverL  = A*EoverL;			// EA/L
  double GJoverL  = G*Jx*oneOverL;         // GJ/L
  
  q(0) = EAoverL*v(0);
  q(5) = GJoverL*v(5);  
  kb.Zero();
  kb(0,0) = EAoverL;
  kb(5,5) = GJoverL;
  if (releasez == 0) {
    double EIzoverL2 = 2.0*Iz*EoverL;		// 2EIz/L
    double EIzoverL4 = 2.0*EIzoverL2;		// 4EIz/L
    q(1) = EIzoverL4*v(1) + EIzoverL2*v(2);
    q(2) = EIzoverL2*v(1) + EIzoverL4*v(2);
    kb(1,1) = kb(2,2) = EIzoverL4;
    kb(2,1) = kb(1,2) = EIzoverL2;
  }
  if (releasez == 1) { // release I
    q(1) = 0.0;
    double EIoverL3 = 3.0*Iz*EoverL;
    q(2) = EIoverL3*v(2);
    kb(2,2) = EIoverL3;
  }
  if (releasez == 2) { // release J
    q(2) = 0.0;
    double EIoverL3 = 3.0*Iz*EoverL;
    q(1) = EIoverL3*v(1);
    kb(1,1) = EIoverL3;
  }
  if (releasez == 3) { // release I and J
    q(1) = 0.0;
    q(2) = 0.0;
  }

  if (releasey == 0) {
    double EIyoverL2 = 2.0*Iy*EoverL;		// 2EIy/L
    double EIyoverL4 = 2.0*EIyoverL2;		// 4EIy/L
    q(3) = EIyoverL4*v(3) + EIyoverL2*v(4);
    q(4) = EIyoverL2*v(3) + EIyoverL4*v(4);    
    kb(3,3) = kb(4,4) = EIyoverL4;
    kb(4,3) = kb(3,4) = EIyoverL2;
  }
  if (releasey == 1) { // release I
    q(3) = 0.0;
    double EIoverL3 = 3.0*Iy*EoverL;
    q(4) = EIoverL3*v(4);
    kb(4,4) = EIoverL3;
  }
  if (releasey == 2) { // release J
    q(4) = 0.0;
    double EIoverL3 = 3.0*Iy*EoverL;
    q(3) = EIoverL3*v(3);
    kb(3,3) = EIoverL3;
  }
  if (releasey == 3) { // release I and J
    q(3) = 0.0;
    q(4) = 0.0;
  }
  
  q(0) += q0[0];
  q(1) += q0[1];
  q(2) += q0[2];
  q(3) += q0[3];
  q(4) += q0[4];
  
  if(theDamping) kb *= theDamping->getStiffnessMultiplier();  

  return theCoordTransf->getGlobalStiffMatrix(kb,q);
}


const Matrix &
ElasticBeam3d::getInitialStiff(void)
{
  //  const Vector &v = theCoordTransf->getBasicTrialDisp();
  
  double L = theCoordTransf->getInitialLength();
  double oneOverL = 1.0/L;
  double EoverL   = E*oneOverL;
  double EAoverL  = A*EoverL;			// EA/L
  double GJoverL  = G*Jx*oneOverL;         // GJ/L

  kb.Zero();
  kb(0,0) = EAoverL;
  kb(5,5) = GJoverL;
  if (releasez == 0) {
    double EIzoverL2 = 2.0*Iz*EoverL;		// 2EIz/L
    double EIzoverL4 = 2.0*EIzoverL2;		// 4EIz/L
    kb(1,1) = kb(2,2) = EIzoverL4;
    kb(2,1) = kb(1,2) = EIzoverL2;
  }
  if (releasez == 1) { // release I
    kb(2,2) = 3.0*Iz*EoverL;
  }
  if (releasez == 2) { // release J
    kb(1,1) = 3.0*Iz*EoverL;
  }

  if (releasey == 0) {
    double EIyoverL2 = 2.0*Iy*EoverL;		// 2EIy/L
    double EIyoverL4 = 2.0*EIyoverL2;		// 4EIy/L
    kb(3,3) = kb(4,4) = EIyoverL4;
    kb(4,3) = kb(3,4) = EIyoverL2;
  }
  if (releasey == 1) { // release I
    kb(4,4) = 3.0*Iy*EoverL;
  }
  if (releasey == 2) { // release J
    kb(3,3) = 3.0*Iy*EoverL;
  }

  if(theDamping) kb *= theDamping->getStiffnessMultiplier();  

  return theCoordTransf->getInitialGlobalStiffMatrix(kb);
}

const Matrix &
ElasticBeam3d::getMass(void)
{ 
    K.Zero();
    
    if (rho > 0.0) {
        // get initial element length
        double L = theCoordTransf->getInitialLength();
        if (cMass == 0)  {
            // lumped mass matrix
            double m = 0.5*rho*L;
            K(0,0) = m;
            K(1,1) = m;
            K(2,2) = m;
            K(6,6) = m;
            K(7,7) = m;
            K(8,8) = m;
        } else  {
            // consistent mass matrix
            static Matrix ml(12,12);
            double m = rho*L/420.0;
            ml(0,0) = ml(6,6) = m*140.0;
            ml(0,6) = ml(6,0) = m*70.0;
            ml(3,3) = ml(9,9) = m*(Jx/A)*140.0;
            ml(3,9) = ml(9,3) = m*(Jx/A)*70.0;

            ml(2,2) = ml(8,8) = m*156.0;
            ml(2,8) = ml(8,2) = m*54.0;
            ml(4,4) = ml(10,10) = m*4.0*L*L;
            ml(4,10) = ml(10,4) = -m*3.0*L*L;
            ml(2,4) = ml(4,2) = -m*22.0*L;
            ml(8,10) = ml(10,8) = -ml(2,4);
            ml(2,10) = ml(10,2) = m*13.0*L;
            ml(4,8) = ml(8,4) = -ml(2,10);

            ml(1,1) = ml(7,7) = m*156.0;
            ml(1,7) = ml(7,1) = m*54.0;
            ml(5,5) = ml(11,11) = m*4.0*L*L;
            ml(5,11) = ml(11,5) = -m*3.0*L*L;
            ml(1,5) = ml(5,1) = m*22.0*L;
            ml(7,11) = ml(11,7) = -ml(1,5);
            ml(1,11) = ml(11,1) = -m*13.0*L;
            ml(5,7) = ml(7,5) = -ml(1,11);
            
            // transform local mass matrix to global system
            K = theCoordTransf->getGlobalMatrixFromLocal(ml);
        }
    }
    
    return K;
}

void 
ElasticBeam3d::zeroLoad(void)
{
  Q.Zero();

  q0[0] = 0.0;
  q0[1] = 0.0;
  q0[2] = 0.0;
  q0[3] = 0.0;
  q0[4] = 0.0;

  p0[0] = 0.0;
  p0[1] = 0.0;
  p0[2] = 0.0;
  p0[3] = 0.0;
  p0[4] = 0.0;

  wx = 0.0;
  wy = 0.0;
  wz = 0.0;
    
  return;
}

int 
ElasticBeam3d::addLoad(ElementalLoad *theLoad, double loadFactor)
{
  int type;
  const Vector &data = theLoad->getData(type, loadFactor);
  double L = theCoordTransf->getInitialLength();

  if (type == LOAD_TAG_Beam3dUniformLoad) {
    double wy = data(0)*loadFactor;  // Transverse
    double wz = data(1)*loadFactor;  // Transverse
    double wx = data(2)*loadFactor;  // Axial (+ve from node I to J)
    
    this->wx += wx;
    this->wy += wy;
    this->wz += wz;    
    
    double Vy = 0.5*wy*L;
    double Mz = Vy*L/6.0; // wy*L*L/12
    double Vz = 0.5*wz*L;
    double My = Vz*L/6.0; // wz*L*L/12
    double P = wx*L;

    // Reactions in basic system
    p0[0] -= P;
    p0[1] -= Vy;
    p0[2] -= Vy;
    p0[3] -= Vz;
    p0[4] -= Vz;

    // Fixed end forces in basic system
    q0[0] -= 0.5*P;
    if (releasez == 0) {
      q0[1] -= Mz;
      q0[2] += Mz;
    }
    if (releasez == 1) {
      q0[2] += wy*L*L/8;
    }
    if (releasez == 2) {
      q0[1] -= wy*L*L/8;
    }
    
    if (releasey == 0) {
      q0[3] += My;
      q0[4] -= My;
    }
    if (releasey == 1) {
      q0[4] -= wz*L*L/8;
    }
    if (releasey == 2) {
      q0[3] += wz*L*L/8;
    }
    
  }
  else if (type == LOAD_TAG_Beam3dPartialUniformLoad) {
	  double wa = data(2) * loadFactor;  // Axial
	  double wy = data(0) * loadFactor;  // Transverse
	  double wz = data(1) * loadFactor;  // Transverse
	  double a = data(3) * L;
	  double b = data(4) * L;
	  double c = 0.5 * (b + a);
	  double cOverL = c / L;

	  double P = wa * (b - a);
	  double Fy = wy * (b - a);
	  double Fz = wz * (b - a);

	  // Reactions in basic system
	  p0[0] -= P;
	  double V1, V2;
	  V1 = Fy * (1.0 - cOverL);
	  V2 = Fy * cOverL;
	  p0[1] -= V1;
	  p0[2] -= V2;
	  V1 = Fz * (1.0 - cOverL);
	  V2 = Fz * cOverL;
	  p0[3] -= V1;
	  p0[4] -= V2;

	  // Fixed end forces in basic system
	  q0[0] -= P * cOverL;
	  double M1, M2;
	  double beta2 = (1 - cOverL) * (1 - cOverL);
	  double alfa2 = (cOverL) * (cOverL);
	  double gamma2 = (b - a) / L;
	  gamma2 *= gamma2;

	  M1 = -wy * (b - a) * (c * beta2 + gamma2 / 12.0 * (L - 3 * (L - c)));
	  M2 = wy * (b - a) * ((L - c) * alfa2 + gamma2 / 12.0 * (L - 3 * c));
	  q0[1] += M1;
	  q0[2] += M2;
	  M1 = -wz * (b - a) * (c * beta2 + gamma2 / 12.0 * (L - 3 * (L - c)));
	  M2 = wz * (b - a) * ((L - c) * alfa2 + gamma2 / 12.0 * (L - 3 * c));
	  q0[3] -= M1;
	  q0[4] -= M2;
  }
  else if (type == LOAD_TAG_Beam3dPointLoad) {
    double Py = data(0)*loadFactor;
    double Pz = data(1)*loadFactor;
    double N  = data(2)*loadFactor;
    double aOverL = data(3);

    if (aOverL < 0.0 || aOverL > 1.0)
      return 0;

    double a = aOverL*L;
    double b = L-a;

    // Reactions in basic system
    p0[0] -= N;
    double V1, V2;
    V1 = Py*(1.0-aOverL);
    V2 = Py*aOverL;
    p0[1] -= V1;
    p0[2] -= V2;
    V1 = Pz*(1.0-aOverL);
    V2 = Pz*aOverL;
    p0[3] -= V1;
    p0[4] -= V2;

    double L2 = 1.0/(L*L);
    double a2 = a*a;
    double b2 = b*b;

    // Fixed end forces in basic system
    q0[0] -= N*aOverL;
    double M1, M2;
    M1 = -a * b2 * Py * L2;
    M2 = a2 * b * Py * L2;
    q0[1] += M1;
    q0[2] += M2;
    M1 = -a * b2 * Pz * L2;
    M2 = a2 * b * Pz * L2;
    q0[3] -= M1;
    q0[4] -= M2;
  }
  else {
    opserr << "ElasticBeam3d::addLoad()  -- load type unknown for element with tag: " << this->getTag() << endln;
    return -1;
  }

  return 0;
}


int
ElasticBeam3d::addInertiaLoadToUnbalance(const Vector &accel)
{
  if (rho == 0.0)
    return 0;

  // get R * accel from the nodes
  const Vector &Raccel1 = theNodes[0]->getRV(accel);
  const Vector &Raccel2 = theNodes[1]->getRV(accel);
	
  if (6 != Raccel1.Size() || 6 != Raccel2.Size()) {
    opserr << "ElasticBeam3d::addInertiaLoadToUnbalance matrix and vector sizes are incompatible\n";
    return -1;
  }

  // want to add ( - fact * M R * accel ) to unbalance
  if (cMass == 0)  {
    // take advantage of lumped mass matrix
    double L = theCoordTransf->getInitialLength();
    double m = 0.5*rho*L;

    Q(0) -= m * Raccel1(0);
    Q(1) -= m * Raccel1(1);
    Q(2) -= m * Raccel1(2);
    
    Q(6) -= m * Raccel2(0);
    Q(7) -= m * Raccel2(1);
    Q(8) -= m * Raccel2(2);
  } else  {
    // use matrix vector multip. for consistent mass matrix
    static Vector Raccel(12);
    for (int i=0; i<6; i++)  {
      Raccel(i)   = Raccel1(i);
      Raccel(i+6) = Raccel2(i);
    }
    Q.addMatrixVector(1.0, this->getMass(), Raccel, -1.0);
  }
  
  return 0;
}



const Vector &
ElasticBeam3d::getResistingForceIncInertia()
{	
  P = this->getResistingForce(); 
  
  if (theDamping) P += this->getDampingForce();

  // add the damping forces if rayleigh damping
  if (alphaM != 0.0 || betaK != 0.0 || betaK0 != 0.0 || betaKc != 0.0)
    P.addVector(1.0, this->getRayleighDampingForces(), 1.0);
    
  if (rho == 0.0)
    return P;

  // add inertia forces from element mass
  const Vector &accel1 = theNodes[0]->getTrialAccel();
  const Vector &accel2 = theNodes[1]->getTrialAccel();    
    
  if (cMass == 0)  {
    // take advantage of lumped mass matrix
    double L = theCoordTransf->getInitialLength();
    double m = 0.5*rho*L;
    
    P(0) += m * accel1(0);
    P(1) += m * accel1(1);
    P(2) += m * accel1(2);
    
    P(6) += m * accel2(0);
    P(7) += m * accel2(1);
    P(8) += m * accel2(2);
  } else  {
    // use matrix vector multip. for consistent mass matrix
    static Vector accel(12);
    for (int i=0; i<6; i++)  {
      accel(i)   = accel1(i);
      accel(i+6) = accel2(i);
    }
    P.addMatrixVector(1.0, this->getMass(), accel, 1.0);
  }
  
  return P;
}


const Vector &
ElasticBeam3d::getResistingForce()
{
  const Vector &v = theCoordTransf->getBasicTrialDisp();
  
  double L = theCoordTransf->getInitialLength();
  double oneOverL = 1.0/L;
  double EoverL   = E*oneOverL;
  double EAoverL  = A*EoverL;			// EA/L
  double GJoverL = G*Jx*oneOverL;         // GJ/L
  
  q(0) = EAoverL*v(0);
  q(5) = GJoverL*v(5);

  if (releasez == 0) {
    double EIzoverL2 = 2.0*Iz*EoverL;		// 2EIz/L
    double EIzoverL4 = 2.0*EIzoverL2;		// 4EIz/L
    q(1) = EIzoverL4*v(1) + EIzoverL2*v(2);
    q(2) = EIzoverL2*v(1) + EIzoverL4*v(2);
  }
  if (releasez == 1) {
    q(1) = 0.0;
    q(2) = 3.0*Iz*EoverL*v(2);
  }
  if (releasez == 2) {
    q(1) = 3.0*Iz*EoverL*v(1);
    q(2) = 0.0;
  }
  if (releasez == 3) {
    q(1) = 0.0;
    q(2) = 0.0;
  }
  
  if (releasey == 0) {
    double EIyoverL2 = 2.0*Iy*EoverL;		// 2EIy/L
    double EIyoverL4 = 2.0*EIyoverL2;		// 4EIy/L
    q(3) = EIyoverL4*v(3) + EIyoverL2*v(4);
    q(4) = EIyoverL2*v(3) + EIyoverL4*v(4);    
  }
  if (releasey == 1) {
    q(3) = 0.0;
    q(4) = 3.0*Iy*EoverL*v(4);
  }
  if (releasey == 2) {
    q(3) = 3.0*Iy*EoverL*v(3);
    q(4) = 0.0;
  }
  if (releasey == 3) {
    q(3) = 0.0;
    q(4) = 0.0;
  }  
  
  q(0) += q0[0];
  q(1) += q0[1];
  q(2) += q0[2];
  q(3) += q0[3];
  q(4) += q0[4];
  
  if (theDamping) theDamping->update(q);

  Vector p0Vec(p0, 5);
  
  //  opserr << q;

  P = theCoordTransf->getGlobalResistingForce(q, p0Vec);

  // subtract external load P = P - Q
  if (rho != 0)
    P.addVector(1.0, Q, -1.0);
  
  return P;
}

const Vector &
ElasticBeam3d::getDampingForce()
{
  theCoordTransf->update();
  
  return theCoordTransf->getGlobalResistingForce(theDamping->getDampingForce(), Vector(5));
}

int
ElasticBeam3d::sendSelf(int cTag, Channel &theChannel)
{
    int res = 0;

    static Vector data(21);
    
    data(0) = A;
    data(1) = E; 
    data(2) = G; 
    data(3) = Jx; 
    data(4) = Iy; 
    data(5) = Iz;     
    data(6) = rho;
    data(7) = cMass;
    data(8) = this->getTag();
    data(9) = connectedExternalNodes(0);
    data(10) = connectedExternalNodes(1);
    data(11) = theCoordTransf->getClassTag();    	

    int dbTag = theCoordTransf->getDbTag();
    
    if (dbTag == 0) {
      dbTag = theChannel.getDbTag();
      if (dbTag != 0)
	theCoordTransf->setDbTag(dbTag);
    }
    
    data(12) = dbTag;
    
    data(13) = alphaM;
    data(14) = betaK;
    data(15) = betaK0;
    data(16) = betaKc;
    data(17) = releasez;
    data(18) = releasey;    

    data(19) = 0;
    data(20) = 0;
    if (theDamping) {
      data(19) = theDamping->getClassTag();
      int dbTag = theDamping->getDbTag();
      if (dbTag == 0) {
        dbTag = theChannel.getDbTag();
        if (dbTag != 0)
	        theDamping->setDbTag(dbTag);
	    }
      data(20) = dbTag;
    }
    
    // Send the data vector
    res += theChannel.sendVector(this->getDbTag(), cTag, data);
    if (res < 0) {
      opserr << "ElasticBeam3d::sendSelf -- could not send data Vector\n";
      return res;
    }

    // Ask the CoordTransf to send itself
    res += theCoordTransf->sendSelf(cTag, theChannel);
    if (res < 0) {
      opserr << "ElasticBeam3d::sendSelf -- could not send CoordTransf\n";
      return res;
    }

    // Ask the Damping to send itself
    if (theDamping) {
      res += theDamping->sendSelf(cTag, theChannel);
      if (res < 0) {
        opserr << "ElasticBeam3d::sendSelf -- could not send Damping\n";
        return res;
      }
    }

    return res;
}

int
ElasticBeam3d::recvSelf(int cTag, Channel &theChannel, FEM_ObjectBroker &theBroker)
{
  int res = 0;
  static Vector data(21);

  res += theChannel.recvVector(this->getDbTag(), cTag, data);
  if (res < 0) {
    opserr << "ElasticBeam3d::recvSelf -- could not receive data Vector\n";
    return res;
  }
  
  A = data(0);
  E = data(1); 
  G = data(2); 
  Jx = data(3); 
  Iy = data(4); 
  Iz = data(5);     
  rho = data(6);
  cMass = (int)data(7);
  this->setTag((int)data(8));
  connectedExternalNodes(0) = (int)data(9);
  connectedExternalNodes(1) = (int)data(10);
  
  alphaM = data(13);
  betaK = data(14);
  betaK0 = data(15);
  betaKc = data(16);
  releasez = (int)data(17);
  releasey = (int)data(18);
  
  // Check if the CoordTransf is null; if so, get a new one
  int crdTag = (int)data(11);
  if (theCoordTransf == 0) {
    theCoordTransf = theBroker.getNewCrdTransf(crdTag);
    if (theCoordTransf == 0) {
      opserr << "ElasticBeam3d::recvSelf -- could not get a CrdTransf3d\n";
      exit(-1);
    }
  }
  
  // Check that the CoordTransf is of the right type; if not, delete
  // the current one and get a new one of the right type
  if (theCoordTransf->getClassTag() != crdTag) {
    delete theCoordTransf;
    theCoordTransf = theBroker.getNewCrdTransf(crdTag);
    if (theCoordTransf == 0) {
      opserr << "ElasticBeam3d::recvSelf -- could not get a CrdTransf3d\n";
      exit(-1);
    }
  }
  
  // Now, receive the CoordTransf
  theCoordTransf->setDbTag((int)data(12));
  res += theCoordTransf->recvSelf(cTag, theChannel, theBroker);
  if (res < 0) {
    opserr << "ElasticBeam3d::recvSelf -- could not receive CoordTransf\n";
    return res;
  }

  // Check if the Damping is null; if so, get a new one
  int dmpTag = (int)data(19);
  if (dmpTag) {
    if (theDamping == 0) {
      theDamping = theBroker.getNewDamping(dmpTag);
      if (theDamping == 0) {
        opserr << "ElasticBeam3d::recvSelf -- could not get a Damping\n";
        exit(-1);
      }
    }
  
    // Check that the Damping is of the right type; if not, delete
    // the current one and get a new one of the right type
    if (theDamping->getClassTag() != dmpTag) {
      delete theDamping;
      theDamping = theBroker.getNewDamping(dmpTag);
      if (theDamping == 0) {
        opserr << "ElasticBeam3d::recvSelf -- could not get a Damping\n";
        exit(-1);
      }
    }
  
    // Now, receive the Damping
    theDamping->setDbTag((int)data(20));
    res += theDamping->recvSelf(cTag, theChannel, theBroker);
    if (res < 0) {
      opserr << "ElasticBeam3d::recvSelf -- could not receive Damping\n";
      return res;
    }
  }
  else {
    if (theDamping) {
      delete theDamping;
      theDamping = 0;
    }
  }
  
  return res;
}

void
ElasticBeam3d::Print(OPS_Stream &s, int flag)
{
	this->getResistingForce(); 
	
	if (flag == -1) {
		int eleTag = this->getTag();
		s << "EL_BEAM\t" << eleTag << "\t";
		s << "\t" << connectedExternalNodes(0) << "\t" << connectedExternalNodes(1);
		s << "\t0\t0.0000000\n";
	}
    
	else if (flag < -1) {
		int counter = (flag + 1) * -1;
		int eleTag = this->getTag();
		const Vector &force = this->getResistingForce();

		double P, MZ1, MZ2, VY, MY1, MY2, VZ, T;
		double L = theCoordTransf->getInitialLength();
		double oneOverL = 1.0 / L;

		P = q(0);
		MZ1 = q(1);
		MZ2 = q(2);
		VY = (MZ1 + MZ2)*oneOverL;
		MY1 = q(3);
		MY2 = q(4);
		VZ = (MY1 + MY2)*oneOverL;
		T = q(5);

		s << "FORCE\t" << eleTag << "\t" << counter << "\t0";
		s << "\t" << -P + p0[0] << "\t" << VY + p0[1] << "\t" << -VZ + p0[3] << endln;
		s << "FORCE\t" << eleTag << "\t" << counter << "\t1";
		s << "\t" << P << ' ' << -VY + p0[2] << ' ' << VZ + p0[4] << endln;
		s << "MOMENT\t" << eleTag << "\t" << counter << "\t0";
		s << "\t" << -T << "\t" << MY1 << "\t" << MZ1 << endln;
		s << "MOMENT\t" << eleTag << "\t" << counter << "\t1";
		s << "\t" << T << ' ' << MY2 << ' ' << MZ2 << endln;
	}
	
	else if (flag == 2) {
		this->getResistingForce(); // in case linear algo

		static Vector xAxis(3);
		static Vector yAxis(3);
		static Vector zAxis(3);

		theCoordTransf->getLocalAxes(xAxis, yAxis, zAxis);

		s << "#ElasticBeamColumn3D\n";
		s << "#LocalAxis " << xAxis(0) << " " << xAxis(1) << " " << xAxis(2);
		s << " " << yAxis(0) << " " << yAxis(1) << " " << yAxis(2) << " ";
		s << zAxis(0) << " " << zAxis(1) << " " << zAxis(2) << endln;

		const Vector &node1Crd = theNodes[0]->getCrds();
		const Vector &node2Crd = theNodes[1]->getCrds();
		const Vector &node1Disp = theNodes[0]->getDisp();
		const Vector &node2Disp = theNodes[1]->getDisp();

		s << "#NODE " << node1Crd(0) << " " << node1Crd(1) << " " << node1Crd(2)
			<< " " << node1Disp(0) << " " << node1Disp(1) << " " << node1Disp(2)
			<< " " << node1Disp(3) << " " << node1Disp(4) << " " << node1Disp(5) << endln;

		s << "#NODE " << node2Crd(0) << " " << node2Crd(1) << " " << node2Crd(2)
			<< " " << node2Disp(0) << " " << node2Disp(1) << " " << node2Disp(2)
			<< " " << node2Disp(3) << " " << node2Disp(4) << " " << node2Disp(5) << endln;

		double N, Mz1, Mz2, Vy, My1, My2, Vz, T;
		double L = theCoordTransf->getInitialLength();
		double oneOverL = 1.0 / L;

		N = q(0);
		Mz1 = q(1);
		Mz2 = q(2);
		Vy = (Mz1 + Mz2)*oneOverL;
		My1 = q(3);
		My2 = q(4);
		Vz = -(My1 + My2)*oneOverL;
		T = q(5);

		s << "#END_FORCES " << -N + p0[0] << ' ' << Vy + p0[1] << ' ' << Vz + p0[3] << ' '
			<< -T << ' ' << My1 << ' ' << Mz1 << endln;
		s << "#END_FORCES " << N << ' ' << -Vy + p0[2] << ' ' << -Vz + p0[4] << ' '
			<< T << ' ' << My2 << ' ' << Mz2 << endln;
	}
	
	if (flag == OPS_PRINT_CURRENTSTATE) {

		this->getResistingForce(); // in case linear algo

		s << "\nElasticBeam3d: " << this->getTag() << endln;
		s << "\tConnected Nodes: " << connectedExternalNodes;
		s << "\tCoordTransf: " << theCoordTransf->getTag() << endln;
		s << "\tmass density:  " << rho << ", cMass: " << cMass << endln;
		s << "\trelease about z:  " << releasez << endln;
		s << "\trelease about y:  " << releasey << endln;		
		double N, Mz1, Mz2, Vy, My1, My2, Vz, T;
		double L = theCoordTransf->getInitialLength();
		double oneOverL = 1.0 / L;

		N = q(0);
		Mz1 = q(1);
		Mz2 = q(2);
		Vy = (Mz1 + Mz2)*oneOverL;
		My1 = q(3);
		My2 = q(4);
		Vz = -(My1 + My2)*oneOverL;
		T = q(5);

		s << "\tEnd 1 Forces (P Mz Vy My Vz T): "
			<< -N + p0[0] << ' ' << Mz1 << ' ' << Vy + p0[1] << ' ' << My1 << ' ' << Vz + p0[3] << ' ' << -T << endln;
		s << "\tEnd 2 Forces (P Mz Vy My Vz T): "
			<< N << ' ' << Mz2 << ' ' << -Vy + p0[2] << ' ' << My2 << ' ' << -Vz + p0[4] << ' ' << T << endln;
	}
	
	if (flag == OPS_PRINT_PRINTMODEL_JSON) {
		s << "\t\t\t{";
		s << "\"name\": " << this->getTag() << ", ";
		s << "\"type\": \"ElasticBeam3d\", ";
		s << "\"nodes\": [" << connectedExternalNodes(0) << ", " << connectedExternalNodes(1) << "], ";
		s << "\"E\": " << E << ", ";
		s << "\"G\": " << G << ", ";
		s << "\"A\": " << A << ", ";
		s << "\"Jx\": " << Jx << ", ";
		s << "\"Iy\": " << Iy << ", ";
		s << "\"Iz\": " << Iz << ", ";
		s << "\"massperlength\": " << rho << ", ";
		s << "\"releasez\": "<< releasez << ", ";
		s << "\"releasey\": "<< releasey << ", ";		
		s << "\"crdTransformation\": \"" << theCoordTransf->getTag() << "\"}";
	}
}

int
ElasticBeam3d::displaySelf(Renderer &theViewer, int displayMode, float fact, const char **modes, int numMode)
{
    static Vector v1(3);
    static Vector v2(3);

    theNodes[0]->getDisplayCrds(v1, fact, displayMode);
    theNodes[1]->getDisplayCrds(v2, fact, displayMode);
    float d1 = 0.0;
    float d2 = 0.0;
    int res = 0;

    if (displayMode > 0 && numMode == 0)
        res += theViewer.drawLine(v1, v2, d1, d1, this->getTag(), 0);
    else if (displayMode < 0)
        return theViewer.drawLine(v1, v2, 0.0, 0.0, this->getTag(), 0);

    if (numMode > 0) {
      // calculate q for potential need below
      this->getResistingForce();
    }

  for (int i=0; i<numMode; i++) {

    const char *theMode = modes[i];
    if (strcmp(theMode, "axialForce") == 0) {
      d1 = q(0); 
      d2 = q(0);;

      res +=theViewer.drawLine(v1, v2, d1, d1, this->getTag(), i);
      
    } else if (strcmp(theMode, "endMoments") == 0) {
      d1 = q(1);
      d2 = q(2);
      static Vector delta(3); delta = v2-v1; delta/=10;
      res += theViewer.drawPoint(v1+delta, d1, this->getTag(), i);
      res += theViewer.drawPoint(v2-delta, d2, this->getTag(), i);
      
    }
  }    

  return res;
}

Response*
ElasticBeam3d::setResponse(const char **argv, int argc, OPS_Stream &output)
{

  Response *theResponse = 0;

  output.tag("ElementOutput");
  output.attr("eleType","ElasticBeam3d");
  output.attr("eleTag",this->getTag());
  output.attr("node1",connectedExternalNodes[0]);
  output.attr("node2",connectedExternalNodes[1]);
  
  // global forces
  if (strcmp(argv[0],"force") == 0 || strcmp(argv[0],"forces") == 0 ||
      strcmp(argv[0],"globalForce") == 0 || strcmp(argv[0],"globalForces") == 0) {


    output.tag("ResponseType","Px_1");
    output.tag("ResponseType","Py_1");
    output.tag("ResponseType","Pz_1");
    output.tag("ResponseType","Mx_1");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","Px_2");
    output.tag("ResponseType","Py_2");
    output.tag("ResponseType","Pz_2");
    output.tag("ResponseType","Mx_2");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","Mz_2");

    theResponse =  new ElementResponse(this, 2, P);

	// local forces
  } else if (strcmp(argv[0],"localForce") == 0 || strcmp(argv[0],"localForces") == 0) {

    output.tag("ResponseType","N_1");
    output.tag("ResponseType","Vy_1");
    output.tag("ResponseType","Vz_1");
    output.tag("ResponseType","T_1");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","N_2");
    output.tag("ResponseType","Vy_2");
    output.tag("ResponseType","Vz_2");
    output.tag("ResponseType","T_2");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","Mz_2");

    theResponse =  new ElementResponse(this, 3, P);

  // basic forces
  } else if (strcmp(argv[0],"basicForce") == 0 || strcmp(argv[0],"basicForces") == 0) {

    output.tag("ResponseType","N");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","Mz_2");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","T");
    
    theResponse = new ElementResponse(this, 4, Vector(6));
  }
  // basic stiffness -
  else if (strcmp(argv[0],"basicStiffness") == 0) {
    
    output.tag("ResponseType","N");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","Mz_2");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","T");    
    
    theResponse =  new ElementResponse(this, 19, Matrix(6,6));
    
  // global damping forces
  } else if (theDamping && (strcmp(argv[0],"globalDampingForce") == 0 || strcmp(argv[0],"globalDampingForces") == 0)) {

    output.tag("ResponseType","Px_1");
    output.tag("ResponseType","Py_1");
    output.tag("ResponseType","Pz_1");
    output.tag("ResponseType","Mx_1");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","Px_2");
    output.tag("ResponseType","Py_2");
    output.tag("ResponseType","Pz_2");
    output.tag("ResponseType","Mx_2");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","Mz_2");

    theResponse =  new ElementResponse(this, 21, P);

	// local damping forces
  } else if (theDamping && (strcmp(argv[0],"localDampingForce") == 0 || strcmp(argv[0],"localDampingForces") == 0)) {

    output.tag("ResponseType","N_1");
    output.tag("ResponseType","Vy_1");
    output.tag("ResponseType","Vz_1");
    output.tag("ResponseType","T_1");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","N_2");
    output.tag("ResponseType","Vy_2");
    output.tag("ResponseType","Vz_2");
    output.tag("ResponseType","T_2");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","Mz_2");

    theResponse =  new ElementResponse(this, 22, P);

  // basic damping forces
  }  else if (theDamping && (strcmp(argv[0],"basicDampingForce") == 0 || strcmp(argv[0],"basicDampingForces") == 0)) {

    output.tag("ResponseType","N");
    output.tag("ResponseType","Mz_1");
    output.tag("ResponseType","Mz_2");
    output.tag("ResponseType","My_1");
    output.tag("ResponseType","My_2");
    output.tag("ResponseType","T");
    
    theResponse = new ElementResponse(this, 23, Vector(6));

  }  else if (strcmp(argv[0],"deformations") == 0 || 
	      strcmp(argv[0],"basicDeformations") == 0) {
    
    output.tag("ResponseType","eps");
    output.tag("ResponseType","theta11");
    output.tag("ResponseType","theta12");
    output.tag("ResponseType","theta21");
    output.tag("ResponseType","theta22");
    output.tag("ResponseType","phi");
    theResponse = new ElementResponse(this, 5, Vector(6));
  }

  else if (strcmp(argv[0],"sectionX") == 0) {
    if (argc > 2) {
      float xL = atof(argv[1]);
      if (xL < 0.0)
	xL = 0.0;
      if (xL > 1.0)
	xL = 1.0;
      if (strcmp(argv[2],"forces") == 0) {
	theResponse = new ElementResponse(this,6,Vector(6));
	Information &info = theResponse->getInformation();
	info.theDouble = xL;
      }
    }   
  }
  
  output.endTag(); // ElementOutput

  if (theResponse == 0)
    theResponse = theCoordTransf->setResponse(argv, argc, output);
  
  return theResponse;
}

int
ElasticBeam3d::getResponse (int responseID, Information &eleInfo)
{
  double N, V, M1, M2, T;
  double L = theCoordTransf->getInitialLength();
  double oneOverL = 1.0/L;
  static Vector Sd(3);
  static Vector Res(12);
  Res = this->getResistingForce();
  static Vector s(6);
  static Matrix kb(6,6);
  
  switch (responseID) {
  case 1: // stiffness
    return eleInfo.setMatrix(this->getTangentStiff());
    
  case 2: // global forces
    return eleInfo.setVector(Res);
    
  case 3: // local forces
    // Axial
    N = q(0);
    P(6) =  N;
    P(0) = -N+p0[0];
    
    // Torsion
    T = q(5);
    P(9) =  T;
    P(3) = -T;
    
    // Moments about z and shears along y
    M1 = q(1);
    M2 = q(2);
    P(5)  = M1;
    P(11) = M2;
    V = (M1+M2)*oneOverL;
    P(1) =  V+p0[1];
    P(7) = -V+p0[2];
    
    // Moments about y and shears along z
    M1 = q(3);
    M2 = q(4);
    P(4)  = M1;
    P(10) = M2;
    V = (M1+M2)*oneOverL;
    P(2) = -V+p0[3];
    P(8) =  V+p0[4];

    return eleInfo.setVector(P);
    
  case 4: // basic forces

    return eleInfo.setVector(q);

  case 5:
    return eleInfo.setVector(theCoordTransf->getBasicTrialDisp());

  case 6: {
    double xL = eleInfo.theDouble;
    double x = xL*L;
    
    s(0) = q(0) + wx*(L-x);
    s(1) = q(1)*(xL-1.0) + q(2)*xL + 0.5*wy*x*(x-L);
    s(2) = (q(1)+q(2))/L + wy*(x-0.5*L);
    s(3) = q(3)*(xL-1.0) + q(4)*xL - 0.5*wz*x*(x-L);
    s(4) = (q(3)+q(4))/L - wz*(x-0.5*L);
    s(5) = q(5);

    return eleInfo.setVector(s);
  }

  case 19: // basic stiffness
    kb.Zero();
    kb(0,0) = E*A/L;
    kb(5,5) = G*Jx/L;
    if (releasez == 0) {
      kb(1,1) = kb(2,2) = 4*E*Iz/L;
      kb(1,2) = kb(2,1) = 2*E*Iz/L;
    }
    if (releasez == 1)
      kb(2,2) = 3*E*Iz/L;
    if (releasez == 2)
      kb(1,1) = 3*E*Iz/L;
    
    if (releasey == 0) {
      kb(3,3) = kb(4,4) = 4*E*Iy/L;
      kb(3,4) = kb(4,3) = 2*E*Iy/L;
    }
    if (releasey == 1)
      kb(4,4) = 3*E*Iy/L;
    if (releasey == 2)
      kb(3,3) = 3*E*Iy/L;        
    return eleInfo.setMatrix(kb);
    
  case 21: // global damping forces
    return eleInfo.setVector(this->getDampingForce());
    
  case 22: // local damping forces
    
    Sd = theDamping->getDampingForce();
    
    // Axial
    N = Sd(0);
    P(6) =  N;
    P(0) = -N;
    
    // Torsion
    T = Sd(5);
    P(9) =  T;
    P(3) = -T;
    
    // Moments about z and shears along y
    M1 = Sd(1);
    M2 = Sd(2);
    P(5)  = M1;
    P(11) = M2;
    V = (M1+M2)*oneOverL;
    P(1) =  V;
    P(7) = -V;
    
    // Moments about y and shears along z
    M1 = Sd(3);
    M2 = Sd(4);
    P(4)  = M1;
    P(10) = M2;
    V = (M1+M2)*oneOverL;
    P(2) = -V;
    P(8) =  V;

    return eleInfo.setVector(P);
    
  case 23: // basic damping forces

    return eleInfo.setVector(theDamping->getDampingForce());

  default:
    break;
  }

  return -1;
}


int
ElasticBeam3d::setParameter(const char **argv, int argc, Parameter &param)
{
  if (argc < 1)
    return -1;

  // E of the beam interior
  if (strcmp(argv[0],"E") == 0) {
    param.setValue(E);
    return param.addObject(1, this);
  }
  // A of the beam interior
  if (strcmp(argv[0],"A") == 0) {
    param.setValue(A);
    return param.addObject(2, this);
  }
  // Iz of the beam interior
  if (strcmp(argv[0],"Iz") == 0) {
    param.setValue(Iz);
    return param.addObject(3, this);
  }
  // Iy of the beam interior
  if (strcmp(argv[0],"Iy") == 0) {
    param.setValue(Iy);
    return param.addObject(4, this);
  }
  // G of the beam interior
  if (strcmp(argv[0],"G") == 0) {
    param.setValue(G);
    return param.addObject(5, this);
  }
  // J of the beam interior
  if (strcmp(argv[0],"J") == 0) {
    param.setValue(Jx);
    return param.addObject(6, this);
  }
  // moment release
  if (strcmp(argv[0],"releasez") == 0) {
    param.setValue(releasez);
    return param.addObject(7, this);
  }
  if (strcmp(argv[0],"releasey") == 0) {
    param.setValue(releasey);
    return param.addObject(8, this);
  }  

  // damping
  if (strstr(argv[0], "damp") != 0) {

    if (argc < 2 || !theDamping)
      return -1;

    return theDamping->setParameter(&argv[1], argc-1, param);
  }
  
  return -1;
}

int
ElasticBeam3d::updateParameter (int parameterID, Information &info)
{
	switch (parameterID) {
	case -1:
		return -1;
	case 1:
		E = info.theDouble;
		return 0;
	case 2:
		A = info.theDouble;
		return 0;
	case 3:
		Iz = info.theDouble;
		return 0;
	case 4:
		Iy = info.theDouble;
		return 0;
	case 5:
		G = info.theDouble;
		return 0;
	case 6:
		Jx = info.theDouble;
		return 0;
	case 7:
	  releasez = (int)info.theDouble;
	  if (releasez < 0 || releasez > 3)
	    releasez = 0;
	  return 0;
	case 8:
	  releasey = (int)info.theDouble;
	  if (releasey < 0 || releasey > 3)
	    releasey = 0;
	  return 0;			  
	default:
		return -1;
	}
}

