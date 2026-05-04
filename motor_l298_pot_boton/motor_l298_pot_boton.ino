// Control de motor DC con L298N
// - Potenciometro: velocidad
// - Boton: cambio de direccion
// Compatible con Arduino UNO y Arduino Nano (misma asignacion de pines)
// Firmware: v1.0.0 (primera version estable)

const char* FIRMWARE_VERSION = "v1.0.0";

const uint8_t PIN_ENA = 5;      // PWM hacia ENA del L298N
const uint8_t PIN_IN1 = 8;      // IN1 del L298N
const uint8_t PIN_IN2 = 9;      // IN2 del L298N
const uint8_t PIN_POT = A0;     // Cursor del potenciometro
const uint8_t PIN_BOTON = 2;    // Boton a GND (usando INPUT_PULLUP)

bool direccionAdelante = true;
bool direccionObjetivoAdelante = true;

const unsigned long RAMPA_CAMBIO_MS = 8000;
bool rampaCambioActiva = false;
bool direccionConmutada = false;
unsigned long inicioRampaMs = 0;
int pwmInicioRampa = 0;
int pwmActual = 0;

// Antirrebote por software
bool estadoBotonEstable = HIGH;
bool ultimaLecturaBoton = HIGH;
unsigned long ultimoCambio = 0;
const unsigned long DEBOUNCE_MS = 35;

void setup() {
  Serial.begin(9600);
  Serial.print("SpoolWinder Firmware ");
  Serial.println(FIRMWARE_VERSION);
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
        iniciarCambioDireccionConRampa();
      }
    }
  }
}

void actualizarVelocidad() {
  int lecturaPot = analogRead(PIN_POT);   // 0..1023
  int pwmObjetivo = map(lecturaPot, 0, 1023, 0, 255);

  // Zona muerta para evitar zumbido a muy baja velocidad
  if (pwmObjetivo < 12) {
    pwmObjetivo = 0;
  }

  if (!rampaCambioActiva) {
    pwmActual = pwmObjetivo;
  } else {
    unsigned long transcurrido = millis() - inicioRampaMs;
    unsigned long mitad = RAMPA_CAMBIO_MS / 2;

    if (transcurrido >= RAMPA_CAMBIO_MS) {
      rampaCambioActiva = false;
      direccionConmutada = false;
      direccionAdelante = direccionObjetivoAdelante;
      aplicarDireccion();
      pwmActual = pwmObjetivo;
    } else if (transcurrido < mitad) {
      // Primera mitad: desacelera linealmente hasta 0
      pwmActual = (int)((long)pwmInicioRampa * (long)(mitad - transcurrido) / (long)mitad);
    } else {
      // Segunda mitad: cambia de direccion y acelera al nuevo objetivo
      if (!direccionConmutada) {
        direccionAdelante = direccionObjetivoAdelante;
        aplicarDireccion();
        direccionConmutada = true;
      }

      unsigned long t2 = transcurrido - mitad;
      pwmActual = (int)((long)pwmObjetivo * (long)t2 / (long)mitad);
    }
  }

  analogWrite(PIN_ENA, pwmActual);

  // DEBUG: imprime cada 300ms para no saturar el puerto serie
  static unsigned long ultimoDebug = 0;
  if (millis() - ultimoDebug > 300) {
    ultimoDebug = millis();
    Serial.print("POT=");
    Serial.print(lecturaPot);
    Serial.print("  PWM=");
    Serial.print(pwmActual);
    Serial.print("  DIR=");
    Serial.println(direccionAdelante ? "ADELANTE" : "ATRAS");
  }
}

void iniciarCambioDireccionConRampa() {
  // Si ya esta haciendo rampa, ignora pulsaciones hasta terminar.
  if (rampaCambioActiva) {
    return;
  }

  direccionObjetivoAdelante = !direccionAdelante;
  rampaCambioActiva = true;
  direccionConmutada = false;
  inicioRampaMs = millis();
  pwmInicioRampa = pwmActual;
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
