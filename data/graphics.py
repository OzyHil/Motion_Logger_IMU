import pandas as pd
import matplotlib.pyplot as plt

# Nome do arquivo CSV
arquivo_csv = "dados.csv"  # altere para o nome do seu arquivo, se for diferente

# Tenta carregar o arquivo
try:
    df = pd.read_csv(arquivo_csv)
except Exception as e:
    print("Erro ao ler o arquivo CSV:", e)
    exit(1)

# Verifica se as colunas esperadas estão presentes
colunas_esperadas = {"numero_amostra", "accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z"}
colunas_encontradas = set(df.columns)

if not colunas_esperadas.issubset(colunas_encontradas):
    print("Erro: Arquivo CSV está faltando colunas esperadas.")
    print("Colunas encontradas:", df.columns.tolist())
    exit(1)

# Eixo X: número da amostra (tempo)
tempo = df["numero_amostra"]

# === Gráfico do Acelerômetro ===
plt.figure(figsize=(10, 5))
plt.plot(tempo, df["accel_x"], label="Accel X")
plt.plot(tempo, df["accel_y"], label="Accel Y")
plt.plot(tempo, df["accel_z"], label="Accel Z")
plt.title("Leituras do Acelerômetro")
plt.xlabel("Amostra")
plt.ylabel("Aceleração (m/s²)")
plt.legend()
plt.grid(True)
plt.tight_layout()

# === Gráfico do Giroscópio ===
plt.figure(figsize=(10, 5))
plt.plot(tempo, df["gyro_x"], label="Gyro X")
plt.plot(tempo, df["gyro_y"], label="Gyro Y")
plt.plot(tempo, df["gyro_z"], label="Gyro Z")
plt.title("Leituras do Giroscópio")
plt.xlabel("Amostra")
plt.ylabel("Velocidade Angular (°/s)")
plt.legend()
plt.grid(True)
plt.tight_layout()

# Mostra os gráficos
plt.show()
