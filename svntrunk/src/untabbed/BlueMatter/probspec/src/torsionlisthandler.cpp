/* Copyright 2001, 2019 IBM Corporation
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
 * following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 // **************************************************************************
// File: torsionlisthandler.cpp
// Date: November 12, 2000
// Author: RSG (taken from probspec.cpp written by Mike Pitman)
// Description: Class implementations for singleton classes implemented
//		probspec.cpp that are used to load (push) parameters
//		into the internal data structures which are then used
//		to create the flat data structure which defines the
//		"problem" for a particular molecular system and force
//		field.
//
// **************************************************************************


#include "BlueMatter/torsionlisthandler.hpp"
#include "BlueMatter/pushparams.hpp"
#include <stdlib.h>
#include <assert.h>

TorsionListHandler* TorsionListHandler::s_instance_p = NULL;

TorsionListHandler::TorsionListHandler()
{}

TorsionListHandler* TorsionListHandler::instance()
{
  if (s_instance_p == NULL)
    {
      s_instance_p = new TorsionListHandler;
      assert(s_instance_p != NULL);
    }
  return(s_instance_p);
}

void TorsionListHandler::push(char *s1, char *s2, char *s3, char *s4,double k0,int wells, double r0)
{
  PushTorsionSpec(s1, s2, s3, s4, k0, wells, r0);
}

void TorsionListHandler::finalize()
{}

