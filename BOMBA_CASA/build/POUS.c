void LOGGER_init__(LOGGER *data__, BOOL retain) {
  __INIT_VAR(data__->EN,__BOOL_LITERAL(TRUE),retain)
  __INIT_VAR(data__->ENO,__BOOL_LITERAL(TRUE),retain)
  __INIT_VAR(data__->TRIG,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->MSG,__STRING_LITERAL(0,""),retain)
  __INIT_VAR(data__->LEVEL,LOGLEVEL__INFO,retain)
  __INIT_VAR(data__->TRIG0,__BOOL_LITERAL(FALSE),retain)
}

// Code part
void LOGGER_body__(LOGGER *data__) {
  // Control execution
  if (!__GET_VAR(data__->EN)) {
    __SET_VAR(data__->,ENO,,__BOOL_LITERAL(FALSE));
    return;
  }
  else {
    __SET_VAR(data__->,ENO,,__BOOL_LITERAL(TRUE));
  }
  // Initialise TEMP variables

  if ((__GET_VAR(data__->TRIG,) && !(__GET_VAR(data__->TRIG0,)))) {
    #define GetFbVar(var,...) __GET_VAR(data__->var,__VA_ARGS__)
    #define SetFbVar(var,val,...) __SET_VAR(data__->,var,__VA_ARGS__,val)

   LogMessage(GetFbVar(LEVEL),(char*)GetFbVar(MSG, .body),GetFbVar(MSG, .len));
  
    #undef GetFbVar
    #undef SetFbVar
;
  };
  __SET_VAR(data__->,TRIG0,,__GET_VAR(data__->TRIG,));

  goto __end;

__end:
  return;
} // LOGGER_body__() 





void AUTO_BOMBAS_init__(AUTO_BOMBAS *data__, BOOL retain) {
  __INIT_VAR(data__->BOMBA_CALLE,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->FLOTADOR_CISTERNA,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->SENSOR_PRESION,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->ARRANQUE_BOMBA_CALLE,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->ARRANQUE_BOMBA_TINACO,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->SENSOR_TINACO,__BOOL_LITERAL(FALSE),retain)
  __INIT_VAR(data__->SENSOR_FLUJO,__BOOL_LITERAL(FALSE),retain)
  TON_init__(&data__->TON0,retain);
  __INIT_VAR(data__->TON1,__time_to_timespec(1, 0, 5, 0, 0, 0),retain)
}

// Code part
void AUTO_BOMBAS_body__(AUTO_BOMBAS *data__) {
  // Initialise TEMP variables

  __SET_VAR(data__->TON0.,IN,,(((!(__GET_VAR(data__->SENSOR_PRESION,)) && __GET_VAR(data__->SENSOR_FLUJO,)) && !(__GET_VAR(data__->FLOTADOR_CISTERNA,))) && __GET_VAR(data__->ARRANQUE_BOMBA_CALLE,)));
  __SET_VAR(data__->TON0.,PT,,__GET_VAR(data__->TON1,));
  TON_body__(&data__->TON0);
  __SET_VAR(data__->,BOMBA_CALLE,,__GET_VAR(data__->TON0.Q,));

  goto __end;

__end:
  return;
} // AUTO_BOMBAS_body__() 





