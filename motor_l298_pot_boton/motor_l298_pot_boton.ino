// Control de motor DC con L298N
// - Potenciometro: velocidad
// - Boton: cambio de direccion
// Compatible con Arduino UNO y Arduino Nano (misma asignacion de pines)

const uint8_t PIN_ENA = 5;      // PWM hacia ENA del L298N
const uint8_t PIN_IN1 = 8;      // IN1 del L298N
const uint8_t PIN_IN2 = 9;      // IN2 del L298N
const uint8_t PIN_POT = A0;     // Cursor del potenciometro
const uint8_t PIN_BOTON = 2;    // Boton a GND (usando INPUT_PULLUP)

bool direccionAdelante = true;

// Antirrebote por software
bool estadoBotonEstable = HIGH;
bool ultimaLecturaBoton = HIGH;
unsigned long ultimoCambio = 0;
const unsigned long DEBOUNCE_MS = 35;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);

  aplicarDireccion();
  analogWrite(PIN_ENA, 0);
}

void loop() {
  manejarBoton();
  actualizarVelocidad();
}

void manejarBoton() {
  bool lectura = digitalRead(PIN_BOTON);

  if (lectura != ultimaLecturaBoton) {
    ultimoCambio = millis();
    ultimaLecturaBoton = lectura;
  }

  if ((millis() - ultimoCambio) > DEBOUNCE_MS) {
    if (lectura != estadoBotonEstable) {
      estadoBotonEstable = lectura;

      // Evento de pulsacion: pasa de HIGH a LOW (boton a GND)
      if (estadoBotonEstable == LOW) {
        direccionAdelante = !direccionAdelante;
        aplicarDireccion();
      }
    }
  }
}

void actualizarVelocidad() {
  int lecturaPot = analogRead(PIN_POT);   // 0..1023
  int pwm = map(lecturaPot, 0, 1023, 0, 255);

  // Zona muerta para evitar zumbido a muy baja velocidad
  if (pwm < 12) {
    pwm = 0;
  }

  analogWrite(PIN_ENA, pwm);

  // DEBUG: imprime cada 300ms para no saturar el puerto serie
  static unsigned long ultimoDebug = 0;
  if (millis() - ultimoDebug > 300) {
    ultimoDebug = millis();
    Serial.print("POT=");
    Serial.print(lecturaPot);
    Serial.print("  PWM=");
    Serial.print(pwm);
    Serial.print("  DIR=");
    Serial.println(direccionAdelante ? "ADELANTE" : "ATRAS");
  }
}

void aplicarDireccion() {
  if (direccionAdelante) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
  }
}
