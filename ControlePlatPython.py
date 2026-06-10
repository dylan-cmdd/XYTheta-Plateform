import serial
import serial.tools.list_ports
import time

# Affiche les ports disponibles
ports = serial.tools.list_ports.comports()
for i, port in enumerate(ports):
    print(f"{i}: {port}")

choix = int(input("Numero du port : "))
nom_port = ports[choix].device

# Connexion
arduino = serial.Serial(nom_port, 115200, timeout=2)
time.sleep(2)  # Attendre que l'Arduino redémarre

print("Connecte !")
print("Commandes : XP, XM, YP, YM, TP, TM, RESET, PAS500, exit")

while True:
    commande = input(">> ")

    if commande == "exit":
        arduino.close()
        break

    # Envoie la commande
    arduino.write((commande + '\n').encode('utf-8'))

    # Lit toutes les réponses disponibles
    time.sleep(1)
    while arduino.in_waiting > 0:
        reponse = arduino.readline().decode('utf-8').strip()
        if reponse:
            print("Position :", reponse)