package db;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.Collection;

public class Database {
    private Map<String, String> data;

    public Database() {
        data = new HashMap<>();
    }

    /**
     * Inserisce una coppia chiave-valore nel database
     * @param key La chiave (stringa)
     * @param value Il valore (stringa)
     * @return Il valore precedente associato alla chiave, o null se non esisteva
     */
    public String insert(String key, String value) {
        if (key == null || value == null) {
            throw new IllegalArgumentException("Chiave e valore non possono essere null");
        }
        
        String previousValue = data.put(key, value);
        System.out.println("[Database] Inserito: " + key + " -> " + value);
        
        return previousValue;
    }

    /**
     * Recupera un valore dalla chiave
     * @param key La chiave da cercare
     * @return Il valore associato alla chiave, o null se non esiste
     */
    public String get(String key) {
        return data.get(key);
    }

    /**
     * Rimuove una coppia dal database
     * @param key La chiave da rimuovere
     * @return Il valore rimosso, o null se la chiave non esisteva
     */
    public String remove(String key) {
        String removedValue = data.remove(key);
        if (removedValue != null) {
            System.out.println("[Database] Rimosso: " + key + " -> " + removedValue);
        }
        return removedValue;
    }

    /**
     * Verifica se una chiave esiste nel database
     * @param key La chiave da verificare
     * @return true se la chiave esiste, false altrimenti
     */
    public boolean containsKey(String key) {
        return data.containsKey(key);
    }

    /**
     * Verifica se un valore esiste nel database
     * @param value Il valore da verificare
     * @return true se il valore esiste, false altrimenti
     */
    public boolean containsValue(String value) {
        return data.containsValue(value);
    }

    /**
     * Restituisce tutte le chiavi
     * @return Set di tutte le chiavi
     */
    public Set<String> getAllKeys() {
        return data.keySet();
    }

    /**
     * Restituisce tutti i valori
     * @return Collection di tutti i valori
     */
    public Collection<String> getAllValues() {
        return data.values();
    }

    /**
     * Restituisce tutte le coppie chiave-valore
     * @return Set di tutte le entry
     */
    public Set<Map.Entry<String, String>> getAllEntries() {
        return data.entrySet();
    }

    /**
     * Restituisce il numero di elementi nel database
     * @return Il numero di coppie chiave-valore
     */
    public int size() {
        return data.size();
    }

    /**
     * Verifica se il database è vuoto
     * @return true se vuoto, false altrimenti
     */
    public boolean isEmpty() {
        return data.isEmpty();
    }

    /**
     * Svuota completamente il database
     */
    public void clear() {
        data.clear();
        System.out.println("[Database] Database svuotato");
    }

    /**
     * Aggiorna un valore esistente (solo se la chiave esiste già)
     * @param key La chiave da aggiornare
     * @param newValue Il nuovo valore
     * @return true se l'aggiornamento è avvenuto, false se la chiave non esisteva
     */
    public boolean update(String key, String newValue) {
        if (data.containsKey(key)) {
            String oldValue = data.put(key, newValue);
            System.out.println("[Database] Aggiornato: " + key + " da '" + oldValue + "' a '" + newValue + "'");
            return true;
        }
        return false;
    }

    /**
     * Inserisce o aggiorna una coppia (upsert)
     * @param key La chiave
     * @param value Il valore
     * @return true se è stato un inserimento, false se è stato un aggiornamento
     */
    public boolean upsert(String key, String value) {
        boolean wasNew = !data.containsKey(key);
        data.put(key, value);
        
        if (wasNew) {
            System.out.println("[Database] Nuovo inserimento: " + key + " -> " + value);
        } else {
            System.out.println("[Database] Aggiornamento: " + key + " -> " + value);
        }
        
        return wasNew;
    }

    /**
     * Stampa tutto il contenuto del database
     */
    public void printAll() {
        System.out.println("[Database] Contenuto completo (" + size() + " elementi):");
        for (Map.Entry<String, String> entry : data.entrySet()) {
            System.out.println("  " + entry.getKey() + " -> " + entry.getValue());
        }
    }

    /**
     * Restituisce una rappresentazione stringa del database
     */
    @Override
    public String toString() {
        return "Database{size=" + size() + ", data=" + data + "}";
    }
}
