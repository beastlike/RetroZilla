/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the PKIX-C library.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are
 * Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems, Inc.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * pkix_defaultcrlchecker.c
 *
 * Functions for default CRL Checkers
 *
 */
#include "pkix.h"
#include "pkix_crlchecker.h"
#include "pkix_tools.h"

/* --Private-CRLChecker-Data-and-Types------------------------------- */

typedef struct pkix_CrlCheckerStruct {
    /* RevocationMethod is the super class of CrlChecker. */
    pkix_RevocationMethod method;
    PKIX_List *certStores; /* list of CertStore */
    PKIX_PL_VerifyCallback crlVerifyFn;
} pkix_CrlChecker;


/* --Private-CRLChecker-Functions----------------------------------- */

/*
 * FUNCTION: pkix_CrlCheckerstate_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_CrlChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_CrlChecker *state = NULL;

        PKIX_ENTER(CRLCHECKER, "pkix_CrlChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        /* Check that this object is a default CRL checker state */
        PKIX_CHECK(
            pkix_CheckType(object, PKIX_CRLCHECKER_TYPE, plContext),
            PKIX_OBJECTNOTCRLCHECKER);

        state = (pkix_CrlChecker *)object;

        PKIX_DECREF(state->certStores);

cleanup:

        PKIX_RETURN(CRLCHECKER);
}

/*
 * FUNCTION: pkix_CrlChecker_RegisterSelf
 *
 * DESCRIPTION:
 *  Registers PKIX_CRLCHECKER_TYPE and its related functions
 *  with systemClasses[]
 *
 * THREAD SAFETY:
 *  Not Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_CrlChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry* entry = &systemClasses[PKIX_CRLCHECKER_TYPE];

        PKIX_ENTER(CRLCHECKER, "pkix_CrlChecker_RegisterSelf");

        entry->description = "CRLChecker";
        entry->typeObjectSize = sizeof(pkix_CrlChecker);
        entry->destructor = pkix_CrlChecker_Destroy;

        PKIX_RETURN(CRLCHECKER);
}

/*
 * FUNCTION: pkix_CrlChecker_Create
 *
 * DESCRIPTION:
 *  Allocate and initialize CRLChecker state data.
 *
 * PARAMETERS
 *  "certStores"
 *      Address of CertStore List to be stored in state. Must be non-NULL.
 *  "testDate"
 *      Address of PKIX_PL_Date to be checked. May be NULL.
 *  "trustedPubKey"
 *      Trusted Anchor Public Key for verifying first Cert in the chain.
 *      Must be non-NULL.
 *  "certsRemaining"
 *      Number of certificates remaining in the chain.
 *  "nistCRLPolicyEnabled"
 *      If enabled, enforce nist crl policy.
 *  "pChecker"
 *      Address of CRLChecker that is returned. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a DefaultCrlChecker Error if the function fails in a
 *  non-fatal way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_CrlChecker_Create(PKIX_RevocationMethodType methodType,
                       PKIX_UInt32 flags,
                       PKIX_UInt32 priority,
                       pkix_LocalRevocationCheckFn localRevChecker,
                       pkix_ExternalRevocationCheckFn externalRevChecker,
                       PKIX_List *certStores,
                       PKIX_PL_VerifyCallback crlVerifyFn,
                       pkix_RevocationMethod **pChecker,
                       void *plContext)
{
        pkix_CrlChecker *crlChecker = NULL;
        
        PKIX_ENTER(CRLCHECKER, "pkix_CrlChecker_Create");
        PKIX_NULLCHECK_TWO(certStores, pChecker);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CRLCHECKER_TYPE,
                    sizeof (pkix_CrlChecker),
                    (PKIX_PL_Object **)&crlChecker,
                    plContext),
                    PKIX_COULDNOTCREATECRLCHECKEROBJECT);

        pkixErrorResult = pkix_RevocationMethod_Init(
            (pkix_RevocationMethod*)crlChecker, methodType, flags,  priority,
            localRevChecker, externalRevChecker, plContext);
        if (pkixErrorResult) {
            goto cleanup;
        }

        /* Initialize fields */
        PKIX_INCREF(certStores);
        crlChecker->certStores = certStores;

        crlChecker->crlVerifyFn = crlVerifyFn;
        *pChecker = (pkix_RevocationMethod*)crlChecker;
        crlChecker = NULL;

cleanup:
        PKIX_DECREF(crlChecker);

        PKIX_RETURN(CRLCHECKER);
}

/* --Private-CRLChecker-Functions------------------------------------ */

/*
 * FUNCTION: pkix_CrlChecker_CheckLocal
 *
 * DESCRIPTION:
 *  Check if the Cert has been revoked based the CRLs data.  This function
 *  maintains the checker state to be current.
 *
 * PARAMETERS
 *  "checker"
 *      Address of CertChainChecker which has the state data.
 *      Must be non-NULL.
 *  "cert"
 *      Address of Certificate that is to be validated. Must be non-NULL.
 *  "unreslvdCrtExts"
 *      A List OIDs. Not **yet** used in this checker function.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Not Thread Safe
 *      (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_CrlChecker_CheckLocal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_Boolean chainVerificationState,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *pReasonCode,
        void *plContext)
{
    PKIX_CertStore_CheckRevokationByCrlCallback storeCheckRevocationFn;
    PKIX_CertStore *certStore = NULL;
    pkix_CrlChecker *state = NULL;
    PKIX_UInt32 reasonCode = 0;
    PKIX_UInt32 crlStoreIndex = 0;
    PKIX_UInt32 numCrlStores = 0;
    PKIX_Boolean storeIsLocal = PKIX_FALSE;
    PKIX_RevocationStatus revStatus = PKIX_RevStatus_NoInfo;

    PKIX_ENTER(CERTCHAINCHECKER, "pkix_CrlChecker_CheckLocal");
    PKIX_NULLCHECK_FOUR(cert, issuer, checkerObject, checkerObject);
    
    state = (pkix_CrlChecker*)checkerObject;

    PKIX_CHECK(
        PKIX_List_GetLength(state->certStores, &numCrlStores, plContext),
        PKIX_LISTGETLENGTHFAILED);

    for (;crlStoreIndex < numCrlStores;crlStoreIndex++) {
        PKIX_CHECK(
            PKIX_List_GetItem(state->certStores, crlStoreIndex,
                              (PKIX_PL_Object **)&certStore,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        
        PKIX_CHECK(
            PKIX_CertStore_GetLocalFlag(certStore, &storeIsLocal,
                                        plContext),
            PKIX_CERTSTOREGETLOCALFLAGFAILED);
        if (storeIsLocal) {
            PKIX_CHECK(
                PKIX_CertStore_GetCrlCheckerFn(certStore,
                                               &storeCheckRevocationFn,
                                               plContext),
                PKIX_CERTSTOREGETCHECKREVBYCRLFAILED);

            if (storeCheckRevocationFn) {
                PKIX_CHECK(
                    storeCheckRevocationFn(certStore, cert, issuer,
                                           date,
                                           /* delay sig check if building
                                            * a chain */
                                           !chainVerificationState,
                                           &reasonCode,
                                           &revStatus, plContext),
                    PKIX_CERTSTORECRLCHECKFAILED);
                if (revStatus == PKIX_RevStatus_Revoked) {
                    break;
                }
            }
        }
        PKIX_DECREF(certStore);
    } /* while */

cleanup:
    *pRevStatus = revStatus;
    PKIX_DECREF(certStore);

    PKIX_RETURN(CERTCHAINCHECKER);
}

/*
 * FUNCTION: pkix_CrlChecker_CheckRemote
 *
 * DESCRIPTION:
 *  Check if the Cert has been revoked based the CRLs data.  This function
 *  maintains the checker state to be current.
 *
 * PARAMETERS
 *  "checker"
 *      Address of CertChainChecker which has the state data.
 *      Must be non-NULL.
 *  "cert"
 *      Address of Certificate that is to be validated. Must be non-NULL.
 *  "unreslvdCrtExts"
 *      A List OIDs. Not **yet** used in this checker function.
 *  "plContext"
 *      Platform-specific context pointer.
 *
 * THREAD SAFETY:
 *  Not Thread Safe
 *      (see Thread Safety Definitions in Programmer's Guide)
 *
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns a CertChainChecker Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error
 */
PKIX_Error *
pkix_CrlChecker_CheckExternal(
        PKIX_PL_Cert *cert,
        PKIX_PL_Cert *issuer,
        PKIX_PL_Date *date,
        pkix_RevocationMethod *checkerObject,
        PKIX_ProcessingParams *procParams,
        PKIX_UInt32 methodFlags,
        PKIX_RevocationStatus *pRevStatus,
        PKIX_UInt32 *pReasonCode,
        void **pNBIOContext,
        void *plContext)
{
    PKIX_CertStore_CheckRevokationByCrlCallback storeCheckRevocationFn = NULL;
    PKIX_CertStore_ImportCrlCallback storeImportCrlFn = NULL;
    PKIX_RevocationStatus revStatus = PKIX_RevStatus_NoInfo;
    PKIX_CertStore *certStore = NULL;
    PKIX_CertStore *localStore = NULL;
    PKIX_CRLSelector *crlSelector = NULL;
    pkix_CrlChecker *state = NULL; 
    PKIX_UInt32 reasonCode = 0;
    PKIX_UInt32 crlStoreIndex = 0;
    PKIX_UInt32 numCrlStores = 0;
    PKIX_Boolean storeIsLocal = PKIX_FALSE;
    PKIX_List *crlList = NULL;
    void *nbioContext = NULL;


    PKIX_ENTER(CERTCHAINCHECKER, "pkix_CrlChecker_CheckExternal");
    PKIX_NULLCHECK_FOUR(cert, issuer, checkerObject, pNBIOContext);
    
    nbioContext = *pNBIOContext;
    *pNBIOContext = NULL; /* prepare for Error exit */

    state = (pkix_CrlChecker*)checkerObject;
    
    PKIX_CHECK(
        PKIX_List_GetLength(state->certStores, &numCrlStores, plContext),
        PKIX_LISTGETLENGTHFAILED);

    /* Find a cert store that is capable of storing crls */
    for (;crlStoreIndex < numCrlStores;crlStoreIndex++) {
        PKIX_CHECK(
            PKIX_List_GetItem(state->certStores, crlStoreIndex,
                              (PKIX_PL_Object **)&certStore,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        
        PKIX_CHECK(
            PKIX_CertStore_GetLocalFlag(certStore, &storeIsLocal,
                                        plContext),
            PKIX_CERTSTOREGETLOCALFLAGFAILED);
        if (storeIsLocal) {
            PKIX_CHECK(
                PKIX_CertStore_GetImportCrlCallback(certStore,
                                                    &storeImportCrlFn,
                                                    plContext),
                PKIX_CERTSTOREGETCHECKREVBYCRLFAILED);
            
            PKIX_CHECK(
                PKIX_CertStore_GetCrlCheckerFn(certStore,
                                               &storeCheckRevocationFn,
                                               plContext),
                PKIX_CERTSTOREGETCHECKREVBYCRLFAILED);
            
            if (storeImportCrlFn && storeCheckRevocationFn) {
                localStore = certStore;
                certStore = NULL;
                break;
            }
        }
        PKIX_DECREF(certStore);
    } /* while */

    /* Report unknown status if we can not check crl in one of the
     * local stores. */
    if (!localStore) {
        PKIX_ERROR_FATAL(PKIX_CRLCHECKERNOLOCALCERTSTOREFOUND);
    }

    PKIX_CHECK(
        PKIX_CrlSelector_Create(issuer, date, &crlSelector, plContext),
        PKIX_CRLCHECKERSETSELECTORFAILED);

    /* Fetch crl and store in a local cert store */
    for (crlStoreIndex = 0;crlStoreIndex < numCrlStores;crlStoreIndex++) {
        PKIX_CertStore_CRLCallback getCrlsFn;

        PKIX_CHECK(
            PKIX_List_GetItem(state->certStores, crlStoreIndex,
                              (PKIX_PL_Object **)&certStore,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        
        PKIX_CHECK(
            PKIX_CertStore_GetCRLCallback(certStore, &getCrlsFn,
                                          plContext),
            PKIX_CERTSTOREGETCRLCALLBACKFAILED);
        
        /* Bit PKIX_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE should
         * be checked in the stores if we have info on missing
         * source in crl case. For now, we are not going to do
         * so, as remote fetching of crls is not implemented. */
        PKIX_CHECK(
            getCrlsFn(certStore, crlSelector, &nbioContext,
                      &crlList, plContext),
            PKIX_GETCRLSFAILED);
        
        PKIX_CHECK(
            storeImportCrlFn(localStore, crlList, plContext),
            PKIX_CERTSTOREFAILTOIMPORTCRLLIST);
        
        PKIX_CHECK(
            storeCheckRevocationFn(certStore, cert, issuer, date,
                                   PKIX_FALSE /* do not delay sig check */,
                                   &reasonCode, &revStatus, plContext),
            PKIX_CERTSTORECRLCHECKFAILED);
        if (revStatus != PKIX_RevStatus_NoInfo) {
            break;
        }
        PKIX_DECREF(crlList);
        PKIX_DECREF(certStore);
    } /* while */

    /* Update return flags */

cleanup:
    if (revStatus == PKIX_RevStatus_NoInfo &&
        methodFlags & PKIX_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE &&
        methodFlags & PKIX_REV_M_FAIL_ON_MISSING_FRESH_INFO) {
        revStatus = PKIX_RevStatus_Revoked;
    }
    *pRevStatus = revStatus;

    PKIX_DECREF(crlList);
    PKIX_DECREF(certStore);
    PKIX_DECREF(localStore);
    PKIX_DECREF(crlSelector);

    PKIX_RETURN(CERTCHAINCHECKER);
}
