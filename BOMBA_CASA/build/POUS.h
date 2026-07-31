#include "beremiz.h"
#ifndef __POUS_H
#define __POUS_H

#include "accessor.h"
#include "iec_std_lib.h"

__DECLARE_ENUMERATED_TYPE(LOGLEVEL,
  LOGLEVEL__CRITICAL,
  LOGLEVEL__WARNING,
  LOGLEVEL__INFO,
  LOGLEVEL__DEBUG
)
// FUNCTION_BLOCK LOGGER
// Data part
typedef struct {
  // FB Interface - IN, OUT, IN_OUT variables
  __DECLARE_VAR(BOOL,EN)
  __DECLARE_VAR(BOOL,ENO)
  __DECLARE_VAR(BOOL,TRIG)
  __DECLARE_VAR(STRING,MSG)
  __DECLARE_VAR(LOGLEVEL,LEVEL)

  // FB private variables - TEMP, private and located variables
  __DECLARE_VAR(BOOL,TRIG0)

} LOGGER;

void LOGGER_init__(LOGGER *data__, BOOL retain);
// Code part
void LOGGER_body__(LOGGER *data__);
// PROGRAM AUTO_BOMBAS
// Data part
typedef struct {
  // PROGRAM Interface - IN, OUT, IN_OUT variables

  // PROGRAM private variables - TEMP, private and located variables
  __DECLARE_VAR(BOOL,BOMBA_CALLE)
  __DECLARE_VAR(BOOL,FLOTADOR_CISTERNA)
  __DECLARE_VAR(BOOL,SENSOR_PRESION)
  __DECLARE_VAR(BOOL,ARRANQUE_BOMBA_CALLE)
  __DECLARE_VAR(BOOL,ARRANQUE_BOMBA_TINACO)
  __DECLARE_VAR(BOOL,SENSOR_TINACO)
  __DECLARE_VAR(BOOL,SENSOR_FLUJO)
  TON TON0;
  __DECLARE_VAR(TIME,TON1)

} AUTO_BOMBAS;

void AUTO_BOMBAS_init__(AUTO_BOMBAS *data__, BOOL retain);
// Code part
void AUTO_BOMBAS_body__(AUTO_BOMBAS *data__);
#endif //__POUS_H
