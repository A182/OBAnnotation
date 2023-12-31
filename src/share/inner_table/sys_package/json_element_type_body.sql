CREATE OR REPLACE TYPE BODY JSON_ELEMENT_T AS
  STATIC FUNCTION parse(jsn VARCHAR2) return   JSON_ELEMENT_T;
  PRAGMA INTERFACE(c, JSON_ELEMENT_PARSE);

  STATIC FUNCTION parse(jsn CLOB)     return   JSON_ELEMENT_T;
  PRAGMA INTERFACE(c, JSON_ELEMENT_PARSE);

  STATIC FUNCTION parse(jsn BLOB)     return   JSON_ELEMENT_T;
  PRAGMA INTERFACE(c, JSON_ELEMENT_PARSE);

  MEMBER FUNCTION to_String    RETURN VARCHAR2;
  PRAGMA INTERFACE(c, JSON_TO_STRING);

  MEMBER FUNCTION to_Number    RETURN NUMBER;
  PRAGMA INTERFACE(c, JSON_TO_NUMBER);

  MEMBER FUNCTION to_Date      RETURN DATE;
  PRAGMA INTERFACE(c, JSON_TO_DATE);

  MEMBER FUNCTION to_Timestamp RETURN TIMESTAMP;
  PRAGMA INTERFACE(c, JSON_TO_TIMESTAMP);

  MEMBER FUNCTION to_Boolean   RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_TO_BOOLEAN);

  MEMBER FUNCTION to_Clob      RETURN CLOB;
  PRAGMA INTERFACE(c, JSON_TO_CLOB);

  MEMBER FUNCTION to_Blob      RETURN BLOB;
  PRAGMA INTERFACE(c, JSON_TO_BLOB);

  MEMBER PROCEDURE to_Clob(c IN OUT CLOB);
  PRAGMA INTERFACE(c, JSON_TO_CLOB_PROC);

  MEMBER PROCEDURE to_Blob(c IN OUT BLOB);
  PRAGMA INTERFACE(c, JSON_TO_BLOB_PROC);

  MEMBER PROCEDURE on_Error(val IN NUMBER);
  PRAGMA INTERFACE(c, JSON_ELEMENT_ON_ERROR);

  MEMBER FUNCTION is_Object    RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_OBJECT);

  MEMBER FUNCTION is_Array     RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_ARRAY);

  MEMBER FUNCTION is_Scalar    RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_SCALAR);

  MEMBER FUNCTION is_String    RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_STRING);

  MEMBER FUNCTION is_Number    RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_NUMBER);

  MEMBER FUNCTION is_Boolean   RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_BOOLEAN);

  MEMBER FUNCTION is_True      RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_TRUE);

  MEMBER FUNCTION is_False     RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_FALSE);

  MEMBER FUNCTION is_Null      RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_NULL);

  MEMBER FUNCTION is_Date      RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_DATE);

  MEMBER FUNCTION is_Timestamp RETURN BOOLEAN;
  PRAGMA INTERFACE(c, JSON_IS_TIMESTAMP);

  MEMBER FUNCTION get_Size     RETURN NUMBER;
  PRAGMA INTERFACE(c, JSON_GET_SIZE);

END;
//