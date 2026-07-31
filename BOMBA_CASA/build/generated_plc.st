PROGRAM Auto_bombas
  VAR
    Bomba_calle : BOOL;
    Flotador_cisterna : BOOL;
    Sensor_presion : BOOL;
    Arranque_bomba_calle : BOOL;
    Arranque_bomba_tinaco : BOOL;
    Sensor_tinaco : BOOL;
    Sensor_flujo : BOOL;
    TON0 : TON;
    TON1 : TIME := T#5s;
  END_VAR

  TON0(IN := NOT(Sensor_presion) AND Sensor_flujo AND NOT(Flotador_cisterna) AND Arranque_bomba_calle, PT := TON1);
  Bomba_calle := TON0.Q;
END_PROGRAM


CONFIGURATION Config0

  RESOURCE Res0 ON PLC
    TASK task0(INTERVAL := T#20ms,PRIORITY := 0);
    PROGRAM instance0 WITH task0 : Auto_bombas;
  END_RESOURCE
END_CONFIGURATION
