use core::time;
use crossterm::execute;
use crossterm::terminal::{Clear, ClearType};
use ndarray::{arr2, s, Array2, AssignElem, Axis};
use serde::{Deserialize, Serialize};
use std::cell::RefCell;
use std::collections::HashMap;
use std::error::Error;
use std::fmt::Display;
use std::fs::File;
use std::io::{stdout, BufReader};
use std::rc::Rc;
use std::thread;
use unicode_segmentation::UnicodeSegmentation;

// TODO Lineas que no sean de 90°
pub fn draw_line(vector: Coordinate) -> Region {
    let (x, y): (usize, usize) = (
        vector.x.abs().try_into().unwrap(),
        vector.y.abs().try_into().unwrap(),
    );
    let mut result = Array2::<Char>::default((x + 1, y + 1));

    result.fill('X'.into());

    result
}

pub fn rectangle(size: Dimension) -> Region {
    let mut rectangle = Array2::<Char>::default((size.x, size.y));

    rectangle.index_axis_mut(Axis(1), 0).fill('X'.into());
    rectangle
        .index_axis_mut(ndarray::Axis(1), size.y - 1)
        .fill('X'.into()); // size.y???

    rectangle.axis_iter_mut(Axis(1)).for_each(|mut line| {
        line[0].assign_elem('X'.into());
        line[size.x - 1].assign_elem('X'.into());
    });

    rectangle
}

#[derive(Clone, Debug)]
pub struct Char {
    val: String,
}

impl Char {
    fn new(val: String) -> Result<Self, Box<dyn Error>> {
        match val.graphemes(true).count() {
            1 => Ok(Char { val: val }),
            0 => Ok(Char { val: " ".into() }),
            _ => Err(format!("{} is not a single character", val).into()),
        }
    }
}

impl Default for Char {
    fn default() -> Self {
        Char::new("".into()).unwrap()
    }
}

impl From<char> for Char {
    fn from(value: char) -> Self {
        Char::new(value.into()).unwrap() // ugh
    }
}

impl Display for Char {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.val)
    }
}

pub trait GameObject {}

pub trait Scriptable {
    fn setup(&mut self);
    fn update(&mut self);
}

pub trait Renderable {
    fn get_visual(&self) -> &Region;
    fn set_visual(&mut self, new: Region);
    fn get_placement(&self) -> &Coordinate;
    fn set_placement(&mut self, new: Coordinate);
    fn get_size(&self) -> &Dimension;
}

pub trait Collidable {
    fn get_collision(&self) -> &Region;
    fn set_collision(&mut self, new: Region);
    fn check_collision(&self) -> Option<String>;
}

#[derive(Debug, Clone)]
pub struct Coordinate {
    x: i32,
    y: i32,
}

#[derive(Debug, Clone)]
pub struct Dimension {
    x: usize,
    y: usize,
}

type Region = Array2<Char>;

pub struct Object {
    placement: Coordinate,
    visual: Region,
    size: Dimension,
    layer: usize,
}

impl GameObject for Object {}

impl Renderable for Object {
    fn get_visual(&self) -> &Region {
        &self.visual
    }

    fn set_visual(&mut self, new: Region) {
        self.size = Dimension {
            x: new.dim().0,
            y: new.dim().1,
        };
        self.visual = new;
    }

    fn get_placement(&self) -> &Coordinate {
        &self.placement
    }

    fn set_placement(&mut self, new: Coordinate) {
        self.placement = new;
    }

    fn get_size(&self) -> &Dimension {
        &self.size
    }
}

impl Object {
    fn new(visual_path: String) -> Result<Object, Box<dyn Error>> {
        let _test_visual = rectangle(Dimension { x: 10, y: 8 });
        let _test_size = Dimension { x: 10, y: 8 };

        Ok(Object {
            placement: Coordinate { x: 8, y: 8 },
            visual: _test_visual,
            size: _test_size,
            layer: 0,
        })
    }
}

type Script = RefCell<Rc<dyn Scriptable>>;
type Render = RefCell<Rc<dyn Renderable>>;

struct Scene {
    objects: HashMap<String, RefCell<Rc<dyn GameObject>>>,
    to_render: HashMap<String, RefCell<Rc<dyn Renderable>>>,
    to_run: HashMap<String, Script>, //???
    pub frame: Region,
}

impl Scene {
    fn new(resolution: Dimension) -> Scene {
        let frame = Array2::<Char>::default((resolution.x, resolution.y));
        Scene {
            objects: HashMap::new(),
            to_render: HashMap::new(),
            to_run: HashMap::new(),
            frame,
        }
    }

    fn new_object(&mut self, name: String) {
        let cube = arr2(&[['X', 'X'], ['X', 'X']]);
        let cube = serde_json::to_string(&cube).unwrap();
        let cube = RefCell::new(Rc::new(Object::new(cube).unwrap()));

        self.objects.insert(name.clone(), cube.clone());
        self.to_render.insert(name, cube.clone());
    }

    fn render(&mut self) {
        for (_, obj) in &self.to_render {
            let obj = obj.borrow();
            let (x_start, x_end) = match obj.get_placement().x {
                x if obj.get_size().x as i32 + x < 0 => return, // TODO agregar warning?
                x if x < 0 => (0, obj.get_size().x + x as usize),
                x => (x, obj.get_size().x + x as usize),
            };

            let (y_start, y_end) = match obj.get_placement().y {
                y if obj.get_size().y as i32 + y < 0 => return,
                y if y < 0 => (0, obj.get_size().y + y as usize),
                y => (y, obj.get_size().y + y as usize),
            };

            let mut obj_region = self
                .frame
                .slice_mut(s![x_start..x_end as i32, y_start..y_end as i32]);

            // obj_region.assign(obj.get().get_visual().view());
            obj_region.assign(&obj.get_visual().view());
        }
    }
}

struct App {
    scene: HashMap<String, Box<Scene>>,
    current_scene: String,
    frametime: u64,
    resolution: Dimension,
}

impl App {
    fn new(framerate: usize, resolution: Dimension, scene: HashMap<String, Box<Scene>>) -> App {
        App {
            scene,
            current_scene: "test_scene".to_owned(),
            frametime: 1000 / framerate as u64,
            resolution,
        }
    }

    pub fn new_scene(&mut self, name: String) {
        let new_scene = Scene::new(self.resolution.clone());
        self.scene.insert(name, Box::new(new_scene));
    }

    fn display(&mut self) {
        // print!("{:?}", self.scene[&self.current_scene]);
        let to_display = self.scene.get_mut(&self.current_scene).unwrap();

        to_display.render();

        for line in to_display.frame.axis_iter(Axis(1)) {
            for char in line {
                print!("{} ", char);
            }
            print!("\n");
        }
    }

    pub fn start(&mut self) -> Result<(), Box<dyn Error>> {
        loop {
            self.display();
            thread::sleep(time::Duration::from_millis(self.frametime));
            let mut stdout = stdout();
            execute!(stdout, Clear(ClearType::All))?;
        }
    }

    pub fn scene(&mut self, name: &str) -> &mut Box<Scene> {
        self.scene.get_mut(name).unwrap()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() -> Result<(), Box<dyn Error>> {
        let mut test_game = App::new(1, Dimension { x: 40, y: 30 }, HashMap::new());

        test_game.new_scene("test_scene".into());

        test_game.scene("test_scene").new_object("Cube".to_owned());

        // test_game.scene("test_scene").objects["Cube"];

        test_game.start()
    }
}
