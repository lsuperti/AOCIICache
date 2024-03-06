#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

/*
 * Compliance level: 0 for relaxed, 1 for strict, 2 for very strict.
 *
 * 0, relaxed: Additional features are allowed. Valid inputs per the specification may produce different results than
 * specified, as a result of additional features.
 * 
 * 1, strict: Additional features are allowed. Valid inputs per the specification will produce results as specified.
 * Additional features that interpret inputs valid per the specification in non-compliant implementation-defined ways
 * are not allowed. Only additional features that rely on inputs that are invalid per the specification are allowed.
 * 
 * 2, Very strict: Additional features are not allowed. Valid inputs per the specification will produce results as
 * specified. All inputs not valid per the specification are invalid.
 */
#define COMPLIANCE_LEVEL 1

#endif