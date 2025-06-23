import streamlit as st
import pandas as pd
import psycopg2
import plotly.express as px

# ------------------ CONFIGURAÇÕES INICIAIS ------------------
st.set_page_config(page_title="Monitor de Sinais Vitais", layout="wide")
st.title(" Dashboard de Monitoramento de Sinais Vitais")

# ------------------ CONEXÃO COM POSTGRES ------------------
def conectar_banco():
    return psycopg2.connect(
        host="localhost",
        database="TCC",
        user="postgres",
        password="Gt@240396"
    )

# ------------------ CONSULTA AOS DADOS ------------------
@st.cache_data
def carregar_dados():
    conn = conectar_banco()
    query = """SELECT id, temperatura, batimentos, spo2, "data" as data_registro
               FROM public.medidas;"""
    df = pd.read_sql_query(query, conn)
    conn.close()
    return df

dados = carregar_dados()

# ------------------ EXIBIÇÃO DA TABELA ------------------
st.subheader(" Tabela de Dados")
st.dataframe(dados, use_container_width=True)

# ------------------ FILTRO DE DATA ------------------
st.sidebar.header("Filtros")
datas = pd.to_datetime(dados['data_registro']).dt.date.unique()
data_escolhida = st.sidebar.selectbox("Selecionar Data:", datas)
df_filtrado = dados[pd.to_datetime(dados['data_registro']).dt.date == data_escolhida]

# ------------------ GRÁFICOS ------------------
col1, col2 = st.columns(2)

with col1:
    fig_temp = px.line(df_filtrado, x="data_registro", y="temperatura",
                       title="Temperatura Corporal (°C)", markers=True)
    st.plotly_chart(fig_temp, use_container_width=True)

with col2:
    fig_bpm = px.line(df_filtrado, x="data_registro", y="batimentos",
                      title=" Batimentos Cardíacos (BPM)", markers=True)
    st.plotly_chart(fig_bpm, use_container_width=True)

fig_spo2 = px.line(df_filtrado, x="data_registro", y="spo2",
                   title=" Saturação de Oxigênio (SpO₂ %)", markers=True)
st.plotly_chart(fig_spo2, use_container_width=True)

# ------------------ RODAPÉ ------------------
st.markdown("---")
st.markdown("Desenvolvido para fins acadêmicos – TCC | Autor: Gustavo Henrique Figueiredo")
