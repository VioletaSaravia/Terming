pub trait App {
    fn add(&mut self, left: usize, right: usize) -> usize;
}

struct TestGame {
    val: usize,
}

impl TestGame {
    fn new(val: usize) -> TestGame {
        TestGame { val }
    }
}

impl App for TestGame {
    fn add(&mut self, left: usize, right: usize) -> usize {
        self.val += left + right;
        self.val
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let mut game = TestGame::new(3);
        let result = game.add(2, 2);
        assert_eq!(result, 7);
    }
}
