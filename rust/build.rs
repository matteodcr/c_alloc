//! Ce fichier permet d'exécuter du code arbitraire juste avant la compilation du code Rust.
//!
//! Ici, la bibliothèque [cc] est utilisée pour compiler et lier du code C au projet Rust. Elle va
//! d'abord compiler le C dans un répertoire temporaire du compilateur Rust, puis – en effet de
//! bord – passer des paramètres additionnels au compilateur Rust pour qu'il "pense" à lier
//! l'archive ELF générée.
//! Cette bibliothèque permet commodément de définir des variables du préprocesseur C, et fait
//! complètement abstraction sur le compilateur utilisé en pratique, peu importe la plateforme.

fn main() {
    let memory_size = (1024 * 1024 * 1024).to_string();

    cc::Build::new()
        .file("../common.c")
        .file("../mem.c")
        .define("MEMORY_SIZE", Some(memory_size.as_str()))
        .define("ALLOCATEUR_ALIGNMENT", Some("16"))
        .include("..")
        .compile("info3_allocateur_rs");
}
