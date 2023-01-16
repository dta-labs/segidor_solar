// Sensor Analógico 
// Permite leer valor RAW o conversión 0 - 5 V

class Analogo {
	private:
		int sensorPin;
		bool raw = true;
		
	public:
		Analogo(int pin, bool range = true){
			sensorPin = pin;
      pinMode(sensorPin, INPUT);
			raw = range;
		}
		
		float getValue(){
			int sensorValue = analogRead(sensorPin);
			if (raw) {
				return sensorValue;
			} else {
				return fmap(sensorValue, 0, 1023, 0.0, 5.0);
			}
		}
		
		float getAnalogValue(){
			int sensorValue = analogRead(sensorPin);
			return sensorValue;
		}
		
		float getTTLValue(){
			int sensorValue = analogRead(sensorPin);
			return fmap(sensorValue, 0, 1023, 0.0, 5.0);
		}
		
		bool getDigitalValue(){
			int sensorValue = analogRead(sensorPin);
			int value = fmap(sensorValue, 0, 1023, 0.0, 5.0);
			if (value > 3) {
				return true;
			} else {
				return false;
			}
		}

		float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
			return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
		}
};
